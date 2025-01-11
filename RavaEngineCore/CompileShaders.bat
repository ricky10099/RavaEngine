forfiles /P src\Framework\Vulkan\RenderSystem\GLSLShaders /s /m *.glsl /c "cmd /c %VULKAN_SDK%\Bin\glslc.exe @fname.glsl -o ../../../../../../Assets/Shaders/@fname.spv"
pause