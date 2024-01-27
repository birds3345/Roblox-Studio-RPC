workspace "Roblox-Studio-RPC"
	architecture "x64"

	configurations {
		"Debug",
		"Release",
	}

	startproject "Roblox-Studio-RPC"

project "Roblox-Studio-RPC"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"

	includedirs {"external/Jsonify/include", "external/discord_game_sdk/include"}

	targetdir "bin/%{cfg.buildcfg}"
	objdir "obj/%{cfg.buildcfg}"

	files {"src/**.h", "src/**.cpp"}

	links {"discord_game_sdk", "Jsonify", "external/discord_game_sdk/lib/discord_game_sdk.dll.lib", "ws2_32.lib"}

	filter "configurations:Debug"
		runtime "Debug"
		optimize "Off"
		symbols "On"
		defines {"DEBUG"}

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
		symbols "Off"

include "external/discord_game_sdk"
include "external/Jsonify"