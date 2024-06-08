workspace "limbo"
	architecture "x64"
	configurations { "Debug", "Release" }
	location "build"
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "vendor/glfw/include"
include "vendor/GLFW"

project "limbo"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	targetdir ("bin/" .. outputdir .. "%/{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	pchheader "lmbpch.h"
	pchsource "src/lmbpch.cpp"
	files { "src/**.h", "src/**.cpp", "main/**.cpp" }
	includedirs { "src", "%{IncludeDir.GLFW}" }
	links { "GLFW", "opengl32.lib" }
	filter "configurations:Debug"
		defines { "L_DEBUG" }
		symbols "On"
	filter "configurations:Release"
		defines { "L_RELEASE" }
		optimize "On"

-- Google Test project
project "gtest"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	targetdir ("bin/" .. outputdir .. "%/{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	files { "vendor/googletest/googletest/src/gtest-all.cc", "vendor/googletest/googlemock/src/gmock-all.cc" }
	includedirs { "vendor/googletest/googletest/include", "vendor/googletest/googletest/src", "vendor/googletest/googletest",
				  "vendor/googletest/googlemock/include", "vendor/googletest/googlemock/src", "vendor/googletest/googlemock"}
	filter "configurations:Debug"
		defines { "L_DEBUG" }
		symbols "On"
	filter "configurations:Release"
		defines { "L_RELEASE" }
		optimize "On"

-- Unit tests project
project "tests"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	targetdir ("bin/" .. outputdir .. "%/{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	files { "tests/**.h", "tests/**.cpp" }
	includedirs { "src", "vendor/googletest/googletest/include", "vendor/googletest/googlemock/include" }
	links { "limbo", "gtest" }
	filter "configurations:Debug"
		defines { "L_DEBUG" }
		symbols "On"
	filter "configurations:Release"
		defines { "L_RELEASE" }
		optimize "On"