project "ImGuizmo"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"
    location "ImGuizmo"

    targetdir("%{prj.name}/bin/" ..outputdir.. "/%{prj.name}")
    objdir("%{prj.name}/bin-int/" ..outputdir.. "/%{prj.name}")

    files{
        "%{prj.name}/GraphEditor.h",
        "%{prj.name}/ImCurveEdit.h",
        "%{prj.name}/ImGradient.h",
        "%{prj.name}/imGuizmo.h",
        "%{prj.name}/ImSequencer.h",
        "%{prj.name}/ImZoomSlider.h",
        "%{prj.name}/GraphEditor.cpp",
        "%{prj.name}/ImCurveEdit.cpp",
        "%{prj.name}/ImGradient.cpp",
        "%{prj.name}/imGuizmo.cpp",
        "%{prj.name}/ImSequencer.cpp",
    }

    includedirs {
        "%{prj.name}",
		"../%{IncludeDir.ImGui}",
		"..",
	}

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"