workspace "Engine"
  architecture "x64"

  configurations 
  {
    "Debug",
    "Release"
  }

  group "Dependencies"
    include "tools/premake"
    include "Engine/external/glfw"
    include "Engine/external/imgui"
  group ""

outputdir = "%{cfg.architecture}-%{cfg.buildcfg}"

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["glfw"] = "%{wks.location}/Engine/external/glfw/include"
IncludeDir["glm"] = "%{wks.location}/Engine/external/glm/include"
IncludeDir["imgui"] = "%{wks.location}/Engine/external/imgui"
IncludeDir["stb_image"] = "%{wks.location}/Engine/external/stb_image"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Libs"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
