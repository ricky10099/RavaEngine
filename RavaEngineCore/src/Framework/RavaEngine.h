#pragma once

#include "Framework/Window.h"
#include "Framework/Vulkan/Context.h"
#include "Framework/Vulkan/Renderer.h"
#include "Framework/Scene.h"

namespace Rava {
class Engine {
   public:
	// static constexpr int WINDOW_WIDTH  = 1280;
	// static constexpr int WINDOW_HEIGHT = 720;

   public:
	Engine();
	~Engine();

	NO_COPY(Engine)

	void Run();
	void LoadScene(std::unique_ptr<Scene> scene);

   private:
	Log m_logger;
	Window m_ravaWindow{"Rava Engine"};
	static std::unique_ptr<Vulkan::Context> m_context;
	Vulkan::Renderer m_renderer{&m_ravaWindow};
	std::unique_ptr<Scene> m_currentScene;
};
}  // namespace Rava
