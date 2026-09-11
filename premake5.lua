workspace "Macaw"
    location ""
    configurations { "Debug", "Release" }
    platforms { "x64" }
    defaultplatform "x64"
    startproject "Macaw"

filter "platforms:x64"
    architecture "x86_64"

filter "system:windows"
    systemversion "latest"

filter "configurations:Debug"
    defines { "_DEBUG" }
    symbols "On"
    runtime "Debug"

filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "Speed"
    runtime "Release"

filter {}

project "Macaw"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    characterset "Unicode"

    staticruntime "Off"
    toolset "msc-v145"

    targetdir "bin/%{cfg.buildcfg}/%{cfg.platform}"
    objdir "bin-int/%{cfg.buildcfg}/%{cfg.platform}"

    includedirs {
        ".",
        "range_v_3",
        "Externals/Include",
    }

    defines {
        "NOMINMAX",
    }

    files {
        "**.h",
        "**.cpp",
        "Macaw.rc",
    }

    -- 파일은 보존하되 생성되는 Macaw 프로젝트에는 포함하지 않는다.
    removefiles {
        "Tests/**",
        "doctest/**",
        "range_v_3/**",
        "rapidjson/**",
    }

    pchheader "PCH.h"
    pchsource "pch.cpp"

    links {
        "DirectXTex",
        "d3d11",
        "dxgi",
        "d3dcompiler",
        "dwmapi",
        "gdi32",
        "imm32",
        "shell32",
        "user32",
        "kernel32",
    }

filter "configurations:Debug"
    libdirs { "Externals/bin/debug" }

filter "configurations:Release"
    libdirs { "Externals/bin/release" }

filter {}

-- ImGui와 SimpleMath는 PCH를 사용하지 않는다.
filter "files:ImGui/**.cpp"
    flags { "NoPCH" }

filter "files:SimpleMath/SimpleMath.cpp"
    flags { "NoPCH" }

filter {}
