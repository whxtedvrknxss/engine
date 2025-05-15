workspace "Engine"
  architecture "x64"

  configurations 
  {
    "debug",
    "release"
  }

outputdir = "%{cfg.architecture}-%{cfg.buildcfg}"

project "Engine"
  location "Engine"
  language "C++"
  kind "ConsoleApp"
  cppdialect "C++20"

  targetdir ("build/bin/" .. outputdir .. "/%{prj.name}")
  objdir ("build/obj/" .. outputdir .. "/%{prj.name}")
  
  files 
  {
    "%{prj.name}/src/**.h",
    "%{prj.name}/src/**.cpp"
  }

  includedirs 
  {
    "%{prj.name}/src"
  }

  filter "system:windows"
    systemversion "latest"

