workspace "Engine"
  architecture "x64"

  configurations 
  {
    "Debug",
    "Release"
  }

  startproject "Sandbox"

outputdir = "%{cfg.architecture}-%{cfg.buildcfg}"

group "Core"
  include "Engine"
group ""

group "Miscelaneous"
  include "Sandbox"
group ""

group "Dependencies"
  include "Engine/external/glfw"
  include "Engine/external/imgui"
group ""

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["glfw"] = "%{wks.location}/Engine/external/glfw/include"
IncludeDir["glm"] = "%{wks.location}/Engine/external/glm"
IncludeDir["imgui"] = "%{wks.location}/Engine/external/imgui"
IncludeDir["stb_image"] = "%{wks.location}/Engine/external/stb_image"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
