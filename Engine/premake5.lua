project "Engine"
  language "C++"
  kind "StaticLib"
  cppdialect "C++20"

  targetdir ("%{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}")
  objdir ("%{wks.location}/build/obj/" .. outputdir .. "/%{prj.name}")

  defines 
  {
    "_CRT_SECURE_NO_WARNINGS"
  }
  
  files 
  {
    "src/Engine/**.h",
    "src/Engine/**.cpp"
  }

  includedirs 
  {
    "src",
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

  filter "system:Windows"
    defines 
    {
        "PLATFORM_WINDOWS"
    }

    files
    {
      "src/Platform/Windows/**.h",
      "src/Platform/Windows/**.cpp",
      "src/Platform/Vulkan/**.h",
      "src/Platform/Vulkan/**.cpp"
    }

  filter "configurations:Debug"
    defines "DEBUG"
    runtime "Debug"
    symbols "on"

  filter "configurations:Release"
    defines "RELEASE"
    runtime "Release"
    optimize "on"


