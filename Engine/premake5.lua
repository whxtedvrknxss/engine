project "Engine"
  location "Engine"
  language "C++"
  kind "ConsoleApp"
  cppdialect "C++20"
  staticruntime "off"

  targetdir ("build/bin/" .. outputdir .. "/%{prj.name}")
  objdir ("build/obj/" .. outputdir .. "/%{prj.name}")
  
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


