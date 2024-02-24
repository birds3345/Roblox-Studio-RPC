#include "Settings.h"

#include <filesystem>
#include <sstream>
#include <fstream>

namespace Settings
{
	std::string getName()
	{
		return "Roblox-Studio-RPC-Settings.json";
	}

	bool fileExists()
	{
		return std::filesystem::exists(std::filesystem::current_path() / getName());
	}

	void makeDefault()
	{
		if (std::filesystem::exists(std::filesystem::current_path() / getName())) return;

		Jsonify::JsonValue root;
		root["ClientId"] = "1199942284653903882";

		root["ConsoleEnabled"] = false;

		root["UseLocalhost"] = true;
		root["LocalhostPort"] = 8129;

		root["ShowScriptName"] = true;
		root["ShowGameName"] = true;

		root["DisabledStates"].setType(Jsonify::JsonValue::Type::Array);


		std::ofstream file(std::filesystem::current_path() / getName());

		Jsonify::StringWriter writer({
			.pretty = true,
		});

		std::string out;

		writer.write(root, out);

		file << out;

		file.close();
	}

	Jsonify::JsonValue getSettings()
	{
		if (!std::filesystem::exists(std::filesystem::current_path() / getName())) return {};

		std::ifstream file(std::filesystem::current_path() / getName());

		std::stringstream stream;
		stream << file.rdbuf();
		
		Jsonify::StringReader reader;

		Jsonify::JsonValue out;

		reader.read(stream.str(), out);

		file.close();

		return out;
	}
}