#pragma once

#include "Framework/Window.h"
#include "Framework/Vulkan/Context.h"
#include "Framework/Vulkan/Renderer.h"
#include "Framework/InputEvents/Event.h"
#include "Framework/Scene.h"
#include "Framework/Timestep.h"

namespace Rava {
class Engine {
   public:
	static Engine* instance;
	glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
	u16 frameLimit       = 144;

	// static constexpr int WINDOW_WIDTH  = 1280;
	// static constexpr int WINDOW_HEIGHT = 720;

   public:
	Engine();
	~Engine();

	NO_COPY(Engine)

	void Run();

	// void OnEvent(Event& event);
	void LoadScene(Shared<Scene> scene);

	GLFWwindow* GetGLFWWindow() { return m_ravaWindow.GetGLFWwindow(); }
	u32 GetCurrentFrameIndex() { return m_renderer.GetFrameIndex(); }

   private:
	Log m_logger;
	std::string m_title = "Rava Engine";
	Window m_ravaWindow{m_title};
	static std::unique_ptr<Vulkan::Context> m_context;
	Vulkan::Renderer m_renderer{&m_ravaWindow};
	Shared<Scene> m_currentScene = nullptr;

	Timestep m_timestep{0ms};
	std::chrono::steady_clock::time_point m_timeLastFrame;
	std::chrono::steady_clock::time_point m_fpsLastUpdateTime;
	float m_targetFrameTime = 0.1f;
	u32 m_frameCount        = 0;

	private:
	void UpdateCameraTransform();
	 void UpdateTitleFPS(std::chrono::steady_clock::time_point newTime);
};
}  // namespace Rava
