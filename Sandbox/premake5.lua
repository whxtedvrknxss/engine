project "Sandbox"
  kind "ConsoleApp"
  language "C++"
  cppdialect "C++20"

  targetdir ("%{wks.location}/build/bin/" .. outputdir .. "/%{prj.name}")
  objdir ("%{wks.location}/build/obj/" .. outputdir .. "/%{prj.name}")

  files 
  {
    "src/**.h",
    "src/**.cpp"
  }

  includedirs 
  {
    "%{wks.location}/Engine/Source",
    "%{wks.location}/Engine/external",
    "%{IncludeDir.glm}"
  }

  links
  {
    "Engine"
  }

  systemversion "latest"

  filter "configurations:Debug"
    defines "DEBUG"
    runtime "Debug"
    symbols "on"

  filter "configurations:Release"
    defines "RELEASE"
    runtime "Release"
    optimize "on"
