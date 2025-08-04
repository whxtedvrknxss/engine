project "Engine"
    language "C++"
    kind "StaticLib"
    cppdialect "C++23"

    targetdir ("%{wks.location}/Build/Bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/Build/Obj/" .. outputdir .. "/%{prj.name}")

    defines 
    {
        "_CRT_SECURE_NO_WARNINGS"
    }
  
    files 
    {
        "Source/Engine/**.h",
        "Source/Engine/**.cpp"
    }

    includedirs 
    {
        "Source",
        IncludeDir["glm"],
        IncludeDir["imgui"],
        IncludeDir["stb_image"],
        IncludeDir["VulkanSDK"],
        IncludeDir["sdl"]
    }

    links 
    {
        "imgui",
        "sdl",
        Library["Vulkan"]
    }

    systemversion "latest"

    postbuildcommands 
    {
        '"%{VULKAN_SDK}/Bin/glslc" "%{prj.location}/Shaders/triangle.vert" -o "%{prj.location}/Shaders/triangle.vert.spv"',
        '"%{VULKAN_SDK}/Bin/glslc" "%{prj.location}/Shaders/triangle.frag" -o "%{prj.location}/Shaders/triangle.frag.spv"',
    }

    filter "system:Windows"
    defines 
    {
        "PLATFORM_WINDOWS",
        "VULKAN_SUPPORTED"
    }

    files
    {
        "Source/Platform/Windows/**.h",
        "Source/Platform/Windows/**.cpp",
        "Source/Platform/Vulkan/**.h",
        "Source/Platform/Vulkan/**.cpp"
    }

    filter "configurations:Debug"
        defines "DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "RELEASE"
        runtime "Release"
        optimize "on"


