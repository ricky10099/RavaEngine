#pragma once

#include "Framework/Window.h"
#include "Framework/Vulkan/Context.h"
#include "Framework/Vulkan/Renderer.h"
#include "Framework/Scene.h"
//#include "Framework/Editor.h"

namespace Rava {
class Engine {
   public:
	static Engine* s_engine;
	// static constexpr int WINDOW_WIDTH  = 1280;
	// static constexpr int WINDOW_HEIGHT = 720;

   public:
	Engine();
	~Engine();

	NO_COPY(Engine)

	void Run();
	void LoadScene(std::unique_ptr<Scene> scene);

	GLFWwindow* GetGLFWWindow() { return m_ravaWindow.GetGLFWwindow(); }
	u32 GetCurrentFrameIndex() { return m_renderer.GetFrameIndex(); }

   private:
	Log m_logger;
	Window m_ravaWindow{"Rava Engine"};
	static std::unique_ptr<Vulkan::Context> m_context;
	Vulkan::Renderer m_renderer{&m_ravaWindow};
	std::unique_ptr<Scene> m_currentScene = nullptr;
	//Editor m_editor{m_renderer.GetRenderPass()->GetGUIRenderPass(), m_renderer.GetImageCount()};
	//std::unique_ptr<Editor> m_editor = nullptr;
};
}  // namespace Rava
