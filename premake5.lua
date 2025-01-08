workspace "RavaEngine"
	architecture "x64"
	startproject "RavaEngineCore"

	configurations{
		"Debug",
		"Release",
	}

outputdir = "%{cfg.buildcfg}"

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"]		= "Externals/GLFW/include"
IncludeDir["spdlog"]	= "Externals/spdlog/include"
IncludeDir["Assimp"]	= "Externals/Assimp/include"
IncludeDir["PhysX"]		= "Externals/PhysX/include"
IncludeDir["CRIWARE"]	= "Externals/CRIWARE/include"
IncludeDir["glm"]		= "Externals/glm"

LibDir = {}
LibDir["Assimp"]	= "Externals/Assimp/lib"
LibDir["PhysX"]		= "Externals/PhysX/lib"
LibDir["CRIWARE"]	= "Externals/CRIWARE/lib"

project "RavaEngineCore"
	location "RavaEngineCore"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "off"

	debugdir "$(SolutionDir)"

	pchheader "ravapch.h"
	pchsource "%{prj.name}/src/ravapch.cpp"

	targetdir ("bin/" ..outputdir.. "/%{prj.name}")
	objdir ("bin-int/" ..outputdir.. "/%{prj.name}")

	files {
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.c",
	}
	
	defines {
		"_CRT_SECURE_NO_WARNINGS",
	}

	includedirs {
		"%{prj.name}/src",
		"%{VULKAN_SDK}/Include",
		"Externals",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.PhysX}",
		"%{IncludeDir.CRIWARE}",
		"%{IncludeDir.glm}",
	}
	
	libdirs {
		"%{VULKAN_SDK}/Lib",
		"%{LibDir.Assimp}/%{cfg.buildcfg}",
		"%{LibDir.PhysX}/%{cfg.buildcfg}",
		"%{LibDir.CRIWARE}",
	}

	links {
		"vulkan-1.lib",
		"GLFW",
		"ImGui",
		"PhysX_64.lib",
		"PhysXCommon_64.lib",
		"PhysXCooking_64.lib",
		"PhysXFoundation_64.lib",
		"PhysXPvdSDK_static_64.lib",
		"PhysXExtensions_static_64.lib",
		"cri_ware_pcx64_le_import.lib",
	}

	postbuildcommands {
		-- "call $(SolutionDir)compileGLSLC.bat",
		"{COPY} ../%{LibDir.PhysX}/%{cfg.buildcfg}/*.dll \"../bin/" ..outputdir.. "/%{prj.name}/\"",
		"{COPY} ../%{LibDir.Assimp}/%{cfg.buildcfg}/*.dll \"../bin/" ..outputdir.. "/%{prj.name}/\"",
		"{COPY} ../%{LibDir.CRI}/*.dll \"../bin/" ..outputdir.. "/%{prj.name}/\"",
	}

	buildoptions {
		"/utf-8",
	}

	filter"system:windows"
		systemversion "latest"
		
		defines {
			"GLFW_INCLUDE_NONE",
		}		

	filter "configurations:Debug"
		defines "RAVA_DEBUG"
		runtime "Debug"
		symbols "on"

		links {			
			"assimp-vc143-mtd.lib",
		}

	filter "configurations:Release"
		defines "RAVA_RELEASE"
		runtime "Release"
		optimize "on"
		
		links {			
			"assimp-vc143-mt.lib",
		}

group "Externals"
		include "Externals/ImGui.lua"
		include "Externals/GLFW.lua"