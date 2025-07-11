workspace "Engine"
    architecture "x64"
    startproject "Sandbox"

    configurations 
    {
        "Debug",
        "Release"
    }

    outputdir = "%{cfg.buildcfg}"

    VULKAN_SDK = os.getenv("VULKAN_SDK")

    IncludeDir = {}
    IncludeDir["glm"] = "%{wks.location}/Engine/external/glm"
    IncludeDir["imgui"] = "%{wks.location}/Engine/external/imgui"
    IncludeDir["sdl"] = "%{wks.location}/Engine/external/sdl/include"
    IncludeDir["stb_image"] = "%{wks.location}/Engine/external/stb_image"
    IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

    LibraryDir = {}
    LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

    Library = {}
    Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"

    group "Core"
        include "Engine"

    group "Miscellaneous"
        include "Sandbox"
        include "Editor"

    group "Dependencies"
        include "Engine/external/imgui"
        include "Engine/external/sdl"
