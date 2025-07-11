workspace "Limbo"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "Playground"

    -- Output directory pattern: bin/Debug-Windows-x64/Limbo
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

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
        "%{prj.location}/include"
    }

    filter "system:windows"
        systemversion "latest"

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
    includedirs { "limbo/include" }
    links { "Limbo" }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"
