project "discord_game_sdk"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"

	includedirs {
		"include"
	}

	targetdir "bin/%{cfg.buildcfg}"
	objdir "obj/%{cfg.buildcfg}"

	files {"include/**.h", "src/**.cpp"}

	filter "configurations:Debug"
		runtime "Debug"
		optimize "Off"
		symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
		symbols "Off"