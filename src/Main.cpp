#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <format>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <tlhelp32.h>

#include "Settings.h"
#include "States.h"

#include "Jsonify.h"
#include "discord.h"

static State state = State::Idle;
static std::string script = "Unknown";
static std::string gameName = "Unknown";
static long long playtestTime = 0;

bool isProcessRunning(const std::wstring& name)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry))
	{
		do {
			if (name == entry.szExeFile)
			{
				CloseHandle(snapshot);

				return true;
			}
		} while (Process32Next(snapshot, &entry));
	}

	CloseHandle(snapshot);

	return false;
}

void sendall(SOCKET socket, const char* buf, int len, int flags)
{
	int totalSent = 0;

	do {
		int sent = send(socket, buf, len, flags);
		if (sent == -1) break;

		totalSent += sent;

	} while (totalSent < len);
}

void serverLoop(SOCKET wsaSocket)
{
	while (true)
	{
		SOCKET clientSocket = accept(wsaSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cout << "Could not accept" << std::endl;

			continue;
		}

		char buffer[4096];

		int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);

		if (bytes <= 0)
		{
			std::cout << "Could not read request" << std::endl;

			continue;
		}

		std::string buff(buffer, bytes);

		size_t index;

		if (buff.find("POST") != 0 || (index = buff.find_last_of("\n")) == std::string::npos)
		{
			std::string header = "HTTP/1.1 400 Bad Request\nContent-Type: text/html\nContent-Length:";
			std::string body = "<html><head><title>Roblox-Studio-Discord-RPC</title></head><body><h1>400 Bad Request</h1></body></html>";

			std::string all = header + std::to_string(body.length()) + "\n\n" + body;

			sendall(clientSocket, all.c_str(), (int)all.size(), 0);

			closesocket(clientSocket);

			continue;
		}

		index++;

		Jsonify::JsonValue root;

		Jsonify::StringReader reader;

		try
		{
			reader.read(buff.substr(index), root);
		}
		catch (...)
		{
			std::string header = "HTTP/1.1 400 Bad Request\nContent-Type: application/json\nContent-Length:0";

			sendall(clientSocket, header.c_str(), (int)header.size(), 0);

			closesocket(clientSocket);

			continue;
		}

		long long timeNow = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		if ((State)root["State"].as<int>() == State::Playtesting)
		{
			playtestTime = timeNow;

			state = State::Playtesting;
		}
		else if (timeNow - playtestTime >= 8)
		{
			state = (State)root["State"].as<int>();
			script = root["Script"].as<std::string>();
			gameName = root["GameName"].as<std::string>();
		}

		std::string header = "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length:0";

		sendall(clientSocket, header.c_str(), (int)header.size(), 0);

		closesocket(clientSocket);
	}
}

int main()
{
	HANDLE mutex = CreateMutexA(NULL, FALSE, "RSD_RPC_MUTEX");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBoxA(NULL, "Roblox Studio Discord RPC is already running", "ALREADY RUNNING", MB_OK | MB_ICONERROR);

		return 0;
	}

	std::cout << "Checking files..." << std::endl;

	if (!std::filesystem::exists(std::filesystem::current_path() / "discord_game_sdk.dll"))
	{
		MessageBoxA(NULL, "discord_game_sdk.dll not found, find it in the project releases or download it from the official Discord website and put it in the working directory", "DLL NOT FOUND", MB_OK | MB_ICONERROR);

		return 0;
	}

	if (!Settings::fileExists())
	{
		int res = MessageBoxA(NULL, "The settings file was not found, do you want to create one in the working directory?", "SETTING FILE NOT FOUND", MB_YESNOCANCEL | MB_ICONINFORMATION);

		switch (res)
		{
		case IDYES:
		{
			Settings::makeDefault();

			MessageBoxA(NULL, "The settings file has been created, rerun this program to start", "RESTART", MB_OK | MB_ICONINFORMATION);

			return 0;
		}
		default:
			return 0;
		}
	}

	Jsonify::JsonValue settings = Settings::getSettings();

	if (settings.getOrDefault("UseLocalhost", true))
	{
		int port = settings.getOrDefault("LocalhostPort", 8129).as<int>();

		std::cout << std::format("Setting up localhost on port {}...", port) << std::endl;

		WSAData wsaData;

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			MessageBoxA(NULL, "WSAStartup failed", "LOCALHOST FAILURE", MB_OK | MB_ICONERROR);

			return 0;
		}

		SOCKET wsaSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (wsaSocket == INVALID_SOCKET)
		{
			MessageBoxA(NULL, "failed to create socket", "LOCALHOST FAILURE", MB_OK | MB_ICONERROR);

			return 0;
		}

		sockaddr_in server;
		server.sin_family = AF_INET;
		inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
		server.sin_port = htons(port);

		if (bind(wsaSocket, (SOCKADDR*)&server, sizeof(server)) != 0)
		{
			MessageBoxA(NULL, "failed to bind socket", "LOCALHOST FAILURE", MB_OK | MB_ICONERROR);

			return 0;
		}

		if (listen(wsaSocket, SOMAXCONN) != 0)
		{
			MessageBoxA(NULL, "failed listen to socket", "LOCALHOST FAILURE", MB_OK | MB_ICONERROR);

			return 0;
		}

		std::cout << "Listening to localhost..." << std::endl;

		std::thread(serverLoop, wsaSocket).detach();
	}

#ifndef DEBUG
	if (!settings.getOrDefault("ConsoleEnabled", false))
	{
		for (int i = 3; i > 0; i--)
		{
			std::cout << std::format("Hiding console in {} second{}...", i, i > 1 ? "s" : "") << std::endl;

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		FreeConsole();
	}
#endif

	bool disabledStates[5] = {};

	if (settings.contains("DisabledStates"))
	{
		for (int i = 0; i < settings["DisabledStates"].size(); i++)
		{
			std::string str = settings["DisabledStates"][i].as<std::string>();
			
			if (str == "Idle")
			{
				disabledStates[(int)State::Idle] = true;
			}
			else if (str == "Scripting")
			{
				disabledStates[(int)State::Scripting] = true;
			}
			else if (str == "Building")
			{
				disabledStates[(int)State::Building] = true;
			}
			else if (str == "Playtesting")
			{
				disabledStates[(int)State::Playtesting] = true;
			}
		}
	}

	discord::ClientId clientId = stoll(settings.getOrDefault("ClientId", "1199942284653903882").as<std::string>());

	bool showGameName = settings.getOrDefault("ShowGameName", true).as<bool>();
	bool showScriptName = settings.getOrDefault("ShowScriptName", true).as<bool>();

	discord::Core* core;

	auto result = discord::Core::Create(clientId, DiscordCreateFlags_Default, &core);

	discord::Activity activity{};
	activity.SetState("Idle");
	activity.SetDetails("");
	activity.GetAssets().SetLargeImage("large");

	bool isOn = false;

	State lastState = State::Idle;
	std::string lastScript = "Unknown";
	std::string lastGameName = "Unknown";

	while (true)
	{
		if (!core)
		{
			MessageBoxA(NULL, "An error occured while running", "ERROR", MB_OK | MB_ICONERROR);

			return 0;
		}
		
		bool isOpen = isProcessRunning(L"RobloxStudioBeta.exe");
		if (!isOn && isOpen || (isOpen && (state != lastState || script != lastScript || gameName != lastGameName)))
		{
			if (!isOn && isOpen)
				activity.GetTimestamps().SetStart(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

			isOn = true;
			lastState = state;
			lastScript = script;
			lastGameName = gameName;
			
			if (showGameName)
				activity.SetDetails(gameName.c_str());
			
			switch (state)
			{
			case State::Idle:
				if (!disabledStates[(int)State::Idle])
					activity.SetState("Idle");
				break;

			case State::Building:
				if (!disabledStates[(int)State::Building])
					activity.SetState("Building");
				break;

			case State::Playtesting:
				if (!disabledStates[(int)State::Playtesting])
					activity.SetState("Playtesting");
				break;

			case State::Scripting:
				if (!disabledStates[(int)State::Scripting])
					activity.SetState((showScriptName ? (std::string("Scripting - ") + script) : std::string("Scripting")).c_str());
				break;

			default:
				activity.SetState("Unknown");
				break;
			}
		
			core->ActivityManager().UpdateActivity(activity, [&isOn](discord::Result res)
			{
				if (res != discord::Result::Ok)
				{
					std::cout << std::format("Error while setting presence ({})", (int)res) << std::endl;

					isOn = false;
				}
			});
		}

		if (isOn && !isOpen)
		{
			isOn = false;

			lastState = State::Idle;
			lastScript = "Unknown";
			lastGameName = "Unknown";

			state = State::Idle;
			script = "Unknown";
			gameName = "Unknown";

			delete core;

			discord::Core::Create(clientId, DiscordCreateFlags_Default, &core);

			//ClearActivity doesn't work?
			//Always errors with InvalidPayload
			/*core->ActivityManager().ClearActivity([&isOn](discord::Result res)
			{
				if (res != discord::Result::Ok)
				{
					std::cout << std::format("Error while clearing presence ({})", (int)res) << std::endl;

					isOn = true;
				}
			});*/
		}
		
		core->RunCallbacks();

		std::this_thread::sleep_for(std::chrono::seconds(2));
	};
}