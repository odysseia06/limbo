workspace "Limbo"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "LimboHost"
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
		pic "On"
		links { "X11", "dl", "pthread" }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

project "Game"
    location "game"
    kind "SharedLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir ("bin/" .. outputdir)      -- put DLL/so beside host
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    files { "%{prj.location}/src/**.cpp", "include/public/**.hpp" }
    includedirs { "include", "limbo/include" }
    links { "Limbo" }

    filter "system:windows"
        systemversion "latest"
        targetextension ".dll"

    filter "system:linux"
        links { "dl", "pthread" }     -- dlopen inside host

    filter "configurations:Debug"
        symbols "On"
    filter "configurations:Release"
        optimize "Speed"
		
project "LimboHost"
    location "host"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir ("bin/" .. outputdir)
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    files { "%{prj.location}/src/**.cpp", "include/public/**.hpp" }
    includedirs { "include", "limbo/include", "third_party/glfw/include" }
	dependson { "Game" }
    links { "Limbo", "GLFW" }

    filter "system:windows"
        systemversion "latest"
        links { "opengl32" }

    filter "system:linux"
        links { "dl", "pthread", "X11", "GL" }
        linkgroups "On"

    filter "configurations:Debug"
        symbols "On"
    filter "configurations:Release"
        optimize "Speed"
