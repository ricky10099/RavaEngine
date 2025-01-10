project "ImGui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"
    location "ImGui"

    targetdir("%{prj.name}/bin/" ..outputdir.. "/%{prj.name}")
    objdir("%{prj.name}/bin-int/" ..outputdir.. "/%{prj.name}")

    files{
        "%{prj.name}/imconfig.h",
        "%{prj.name}/imgui.h",
        "%{prj.name}/imgui.cpp",
        "%{prj.name}/imgui_draw.cpp",
        "%{prj.name}/imgui_internal.h",
        "%{prj.name}/imgui_tables.cpp",
        "%{prj.name}/imgui_widgets.cpp",
        "%{prj.name}/imstb_rectpack.h",
        "%{prj.name}/imstb_textedit.h",
        "%{prj.name}/imstb_truetype.h",
        "%{prj.name}/imgui_demo.cpp",
        "%{prj.name}/backends/imgui_impl_vulkan.h",
        "%{prj.name}/backends/imgui_impl_vulkan.cpp",
        "%{prj.name}/backends/imgui_impl_glfw.h",
        "%{prj.name}/backends/imgui_impl_glfw.cpp",
    }

    includedirs {
        "%{prj.name}",
		"%{VULKAN_SDK}/Include",
		"../%{IncludeDir.GLFW}",
	}

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"