project "Engine"
  language "C++"
  kind "StaticLib"
  cppdialect "C++20"
  staticruntime "off"

  targetdir ("%{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}")
  objdir ("%{wks.location}/build/obj/" .. outputdir .. "/%{prj.name}")
  
  files 
  {
    "src/**.h",
    "src/**.cpp"
  }

  includedirs 
  {
    "src",
    "%{IncludeDir.glfw}",
    "%{IncludeDir.glm}",
    "%{IncludeDir.imgui}",
    "%{IncludeDir.stb_image}",
    "%{IncludeDir.VulkanSDK}"
  }

  links 
  {
    "glfw",
    "glm",
    "imgui",
    "stb_image"
  }

  systemversion "latest"

  filter "configurations:Debug"
    defines "Debug"
    runtime "Debug"
    symbols "on"

  filter "configurations:Release"
    defines "RELEASE"
    runtime "Release"
    optimize "on"


