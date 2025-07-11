project "Editor"
  kind "ConsoleApp"
  language "C++"
  cppdialect "C++20"

  targetdir ("%{wks.location}/Build/Bin/" .. outputdir .. "/%{prj.name}")
  objdir ("%{wks.location}/Build/Obj/" .. outputdir .. "/%{prj.name}")

  files 
  {
    "Source/**.h",
    "Source/**.cpp"
  }

  includedirs 
  {
    "%{wks.location}/Engine/Source",
    "%{wks.location}/Engine/external/imgui"
  }

  links 
  {
    "Engine",
    "imgui"
  }

  systemversion "latest"

  filter  "configurations:Debug"
    defines "DEBUG"
    runtime "Debug"
    symbols "On"

  filter "configurations:Release"
    defines "RELEASE"
    runtime "Release"
    optimize "on"
