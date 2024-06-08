workspace "limbo"
	architecture "x64"
	configurations { "Debug", "Release" }
	location "build"

project "limbo"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"
	files { "src/**.h", "src/**.cpp", "main/**.cpp" }
	includedirs { "src" }
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

-- Google Test project
project "gtest"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"
	files { "vendor/googletest/googletest/src/gtest-all.cc", "vendor/googletest/googlemock/src/gmock-all.cc" }
	includedirs { "vendor/googletest/googletest/include", "vendor/googletest/googletest/src", "vendor/googletest/googletest",
				  "vendor/googletest/googlemock/include", "vendor/googletest/googlemock/src", "vendor/googletest/googlemock"}
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

-- Unit tests project
project "tests"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"
	files { "tests/**.h", "tests/**.cpp" }
	includedirs { "src", "vendor/googletest/googletest/include", "vendor/googletest/googlemock/include" }
	links { "limbo", "gtest" }
	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"