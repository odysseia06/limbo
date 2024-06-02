workspace "limbo"
	architecture "x64"
	configurations { "Debug", "Release" }
	location "build"

project "limbo"
	kind "ConsoleApp"
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"
	files { "src/**.h", "src/**.cpp", "main/**.cpp" }
	includedirs { "src" }
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"