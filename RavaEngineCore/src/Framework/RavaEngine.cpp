#include "ravapch.h"

#include "Framework/RavaEngine.h"
#include "Framework/InputEvents/KeyEvent.h"
#include "Framework/InputEvents/MouseEvent.h"
#include "Framework/Resources/Texture.h"
#include "Framework/Camera.h"
#include "Framework/Input.h"
#include "Framework/Entity.h"
#include "Framework/Components.h"

namespace Rava {
Engine* Engine::s_Instance = nullptr;
std::unique_ptr<Vulkan::Context> Engine::m_context;

Engine::Engine() {
	if (s_Instance == nullptr) {
		s_Instance = this;
	} else {
		ENGINE_CRITICAL("Engine already running!");
	}

	m_context = std::make_unique<Vulkan::Context>(&m_ravaWindow);
	m_renderer.Init();
	m_editorCamera.SetPerspectiveProjection(
		glm::radians(50.f), static_cast<float>(m_ravaWindow.Width()) / m_ravaWindow.Height(), 0.1f, 100.f
	);
	m_editorCamera.SetViewYXZ(m_editorCameraPosition, m_editorCameraRotation);
}

Engine::~Engine() {
	ENGINE_INFO("Destruct Engine");
}

void Engine::Run() {
	if (m_currentScene == nullptr) {
		LoadScene(std::make_unique<Scene>("Scene"));
	}

	m_timeLastFrame = std::chrono::high_resolution_clock::now();
	while (!m_ravaWindow.ShouldClose()) {
		if (frameLimit != 0) {
			m_targetFrameTime = 1.0f / frameLimit;
		}
		auto newTime    = std::chrono::high_resolution_clock::now();
		m_timestep      = newTime - m_timeLastFrame;
		m_timeLastFrame = newTime;

		switch (engineState) {
			case EngineState::Run:
				UpdateSceneCamera();
				UpdateSceneAndEntities();
#if RAVA_DEBUG
				RunButton();
#endif
				break;
			case EngineState::Debug:
				UpdateSceneAndEntities();
				[[fallthrough]];
			case EngineState::Edit:
				EditorInputHandle();
				UpdateEditorCamera();
				break;
		}

		m_renderer.BeginFrame();
		m_renderer.UpdateEditor(m_currentScene.get());
		m_renderer.UpdateAnimations(m_currentScene->GetRegistry());
		m_renderer.RenderpassEntities(m_currentScene->GetRegistry(), m_mainCamera);
		m_renderer.RenderEntities(m_currentScene.get());
		m_renderer.RenderEnv(m_currentScene->GetRegistry());

		m_renderer.RenderpassGUI();
		m_renderer.EndScene();

		m_frameCount++;
		UpdateTitleFPS(newTime);
		glfwPollEvents();
	}
	vkDeviceWaitIdle(VKContext->GetLogicalDevice());
}

void Engine::LoadScene(Unique<Scene> scene) {
	if (m_currentScene) {
		vkDeviceWaitIdle(VKContext->GetLogicalDevice());
		m_currentScene.reset();
	}
	m_currentScene = std::move(scene);
	ENGINE_INFO("Initializing {0}", m_currentScene->GetName());
	m_currentScene->Init();
}

void Engine::UpdateTitleFPS(std::chrono::steady_clock::time_point newTime) {
	float elapsedTime = std::chrono::duration<float>(newTime - m_fpsLastUpdateTime).count();
	if (elapsedTime >= 1.0f) {
		int fps = static_cast<int>(m_frameCount / elapsedTime);

		// Update window title with FPS
		std::string windowTitle = m_title + " - FPS: " + std::to_string(fps);
		glfwSetWindowTitle(GetGLFWWindow(), windowTitle.c_str());

		// Reset counters
		m_frameCount        = 0;
		m_fpsLastUpdateTime = newTime;
	}
}

void Engine::EditorInputHandle() {
	if (Input::IsMouseButtonDown(Mouse::Button1)) {
		m_mouseRotateStartPos = Input::GetMousePosition();
	}

	if (Input::IsMouseButtonPress(Mouse::Button1)) {
		if (Input::IsKeyPress(Key::W)) {
			m_editorCameraPosition += glm::vec3{0.0, 0.0, -5.0f} * Timestep::Count();
		}
		if (Input::IsKeyPress(Key::S)) {
			m_editorCameraPosition += glm::vec3{0.0, 0.0, 5.0f} * Timestep::Count();
		}
		if (Input::IsKeyPress(Key::A)) {
			m_editorCameraPosition += glm::vec3{-5.0, 0.0, 0.0f} * Timestep::Count();
		}
		if (Input::IsKeyPress(Key::D)) {
			m_editorCameraPosition += glm::vec3{5.0, 0.0, 0.0f} * Timestep::Count();
		}
		if (Input::IsKeyPress(Key::Q)) {
			m_editorCameraPosition += glm::vec3{0.0, 5.0, 0.0f} * Timestep::Count();
		}
		if (Input::IsKeyPress(Key::E)) {
			m_editorCameraPosition += glm::vec3{0.0, -5.0, 0.0f} * Timestep::Count();
		}

		if (Input::IsMouseButtonDown()) {
			m_mouseTranslateStartPos = Input::GetMousePosition();
		}

		if (Input::IsMouseButtonPress()) {
			m_editorCameraPosition.z +=
				(Input::GetMouseY() - m_mouseTranslateStartPos.y) / m_ravaWindow.Height() * 5.0f * Timestep::Count();
			m_editorCameraPosition.y -=
				(Input::GetMouseX() - m_mouseTranslateStartPos.x) / m_ravaWindow.Width() * 5.0f * Timestep::Count();
		} else {
			m_editorCameraRotation.x -=
				(Input::GetMouseY() - m_mouseRotateStartPos.y) / m_ravaWindow.Height() * 5.0f * Timestep::Count();
			m_editorCameraRotation.y -=
				(Input::GetMouseX() - m_mouseRotateStartPos.x) / m_ravaWindow.Width() * 5.0f * Timestep::Count();
		}
	}

	if (Input::IsKeyDown(Key::F5)) {
		if (engineState == EngineState::Edit) {
			engineState = EngineState::Debug;
		} else if (engineState == EngineState::Debug) {
			engineState = EngineState::Edit;
			vkDeviceWaitIdle(VKContext->GetLogicalDevice());
			m_currentScene->ClearScene();
			auto scene = std::move(m_currentScene);
			LoadScene(std::move(scene));
		}
	}

	RunButton();
}

void Engine::RunButton() {
	if (Input::IsKeyDown(Key::F6)) {
		if (engineState == EngineState::Edit) {
			engineState = EngineState::Run;
		} else if (engineState == EngineState::Run) {
			engineState = EngineState::Edit;
			vkDeviceWaitIdle(VKContext->GetLogicalDevice());
			m_currentScene->ClearScene();
			auto scene = std::move(m_currentScene);
			LoadScene(std::move(scene));
		}
	}
}

void Engine::UpdateEditorCamera() {
	m_mainCamera = m_editorCamera;

	m_editorCamera.MoveCamera(m_editorCameraPosition, m_editorCameraRotation);
	m_editorCamera.RecalculateProjection();
}

void Engine::UpdateSceneAndEntities() {
	m_currentScene->Update();
	for (auto& entity : m_currentScene->GetAllEntities()) {
		entity->Update();
	}
}

void Engine::UpdateSceneCamera() {
	for (auto [entity, cam, transform] : m_currentScene->GetRegistry().view<Component::Camera, Component::Transform>().each()) {
		if (cam.mainCamera) {
			m_mainCamera = cam.view;
		}
		cam.view.MoveCamera(transform.position, transform.rotation);
		cam.view.RecalculateProjection();
	}
}
}  // namespace Rava
