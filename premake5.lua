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
	"**.cc",
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

project "MacawTests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    characterset "Unicode"

    staticruntime "Off"
    toolset "msc-v145"

    targetdir "bin/%{cfg.buildcfg}/%{cfg.platform}"
    objdir "bin-int/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"

    includedirs {
        ".",
        "Externals/Include",
    }

    defines {
        "NOMINMAX",
    }

    -- 테스트는 실제 엔진 소스를 함께 링크하되, Win32/ImGui 에디터 진입점은 제외한다.
    files {
        "**.h",
        "Core/**.cpp",
        "Scene/**.cpp",
        "Serialize/**.cpp",
        "SimpleMath/**.cpp",
        "ErrorHandler.cpp",
        "pch.cpp",
        "Tests/**.cpp",
    }

    removefiles {
        "Tests/TestUndo.cpp", -- 오래된 중복 doctest main 및 폐기된 include 경로
        "ImGui/**",
        "Externals/**",
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

filter "files:SimpleMath/SimpleMath.cpp"
    enablepch "Off"

filter "files:Tests/**.cpp"
    enablepch "Off"

filter {}

-- MacawTests 설정 이후 Macaw 전용 파일 필터를 다시 선택한다.
project "Macaw"

-- ImGui와 SimpleMath는 PCH를 사용하지 않는다.
filter "files:ImGui/**.cpp"
    flags { "NoPCH" }

filter "files:SimpleMath/SimpleMath.cpp"
    flags { "NoPCH" }

filter {}
