#include "ravapch.h"

#include "Framework/RavaEngine.h"
#include "Framework/InputEvents/KeyEvent.h"
#include "Framework/InputEvents/MouseEvent.h"
#include "Framework/Components.h"

namespace Rava {
Engine* Engine::instance = nullptr;
std::unique_ptr<Vulkan::Context> Engine::m_context;

Engine::Engine() {
	if (instance == nullptr) {
		instance = this;
	} else {
		ENGINE_CRITICAL("Engine already running!");
	}

	m_context = std::make_unique<Vulkan::Context>(&m_ravaWindow);
	m_renderer.Init();
}

Engine::~Engine() {
	ENGINE_INFO("Destruct Engine");
}

void Engine::Run() {
	if (m_currentScene == nullptr) {
		LoadScene(std::make_unique<Scene>("No scene"));
	}

	m_timeLastFrame   = std::chrono::high_resolution_clock::now();
	while (!m_ravaWindow.ShouldClose()) {
		if (frameLimit != 0) {
			m_targetFrameTime = 1.0f / frameLimit;
		}
		auto newTime    = std::chrono::high_resolution_clock::now();
		m_timestep      = newTime - m_timeLastFrame;
		m_timeLastFrame = newTime;

		m_currentScene->Update();
		UpdateCameraTransform();

		m_renderer.BeginFrame();
		m_renderer.RenderpassEntities(m_currentScene->GetRegistry());
		m_renderer.RenderEntities(m_currentScene);
		m_renderer.RenderEnv(m_currentScene->GetRegistry());

		m_renderer.UpdateEditor(m_currentScene);
		m_renderer.EndScene();

		m_frameCount++;
		UpdateTitleFPS(newTime);
		glfwPollEvents();
	}
	vkDeviceWaitIdle(VKContext->GetLogicalDevice());
}

void Engine::LoadScene(Shared<Scene> scene) {
	m_currentScene = std::move(scene);
	ENGINE_INFO("Initializing {0}", m_currentScene->GetName());
	m_currentScene->Init();
}

void Engine::UpdateCameraTransform() {
	for (auto [entity, cam, transform] : m_currentScene->GetRegistry().view<Component::Camera, Component::Transform>().each()) {
		if (cam.smoothTranslate) {
			// Smoothly update the camera view
			cam.view.SetTargetViewYXZ(transform.position, transform.rotation);
			cam.view.UpdateView(Timestep::Count());
		} else {
			// Immediate update
			cam.view.SetViewYXZ(transform.position, transform.rotation);
		}

		cam.view.SetViewYXZ(transform.position, transform.rotation);
	}
}

void Engine::UpdateTitleFPS(std::chrono::steady_clock::time_point newTime) {
	float elapsedTime = std::chrono::duration<float>(newTime - m_fpsLastUpdateTime).count();
	if (elapsedTime >= 1.0f) {
		int fps = static_cast<int>(m_frameCount / elapsedTime);

		// Update window title with FPS
		std::string windowTitle = m_title + " - FPS: " + std::to_string(fps);
		glfwSetWindowTitle(GetGLFWWindow(), windowTitle.c_str());

		// Reset counters
		m_frameCount = 0;
		m_fpsLastUpdateTime = newTime;
	}
}
}  // namespace Rava
