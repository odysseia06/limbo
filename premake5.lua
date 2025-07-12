workspace "Limbo"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "Playground"
	location ("build/" .. _ACTION)
    -- Output directory pattern: bin/Debug-Windows-x64/Limbo
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	
	include "third_party/glfw"
	
	filter "configurations:Debug"
        defines { "LIMBO_ENABLE_ASSERTS" }
    filter {}  -- reset filters
	
project "Limbo"
    location "limbo"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir ("bin/"    .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/".. outputdir .. "/%{prj.name}")

    files {
        "%{prj.location}/src/**.cpp",
        "%{prj.location}/include/**.hpp",
        "%{prj.location}/include/**.h"
    }

    includedirs {
        "%{prj.location}/include",
		"third_party/glfw/include"
    }

	links { "GLFW" }

    filter "system:windows"
        systemversion "latest"
		links { "opengl32" }

	filter "system:linux"
		links { "X11", "dl", "pthread" }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

project "Playground"
    location "playground"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir ("bin/"    .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/".. outputdir .. "/%{prj.name}")

    files { "%{prj.location}/src/**.cpp" }
    includedirs { "limbo/include", "third_party/glfw/include" }
    links { "Limbo", "GLFW" }
	linkgroups "On"

    filter "system:windows"
        systemversion "latest"
		links { "opengl32" }
	
	filter "system:linux"
		links { "X11", "dl", "pthread" }
	
    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"
