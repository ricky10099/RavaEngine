#pragma once

#include "Framework/Window.h"
#include "Framework/Vulkan/Context.h"
#include "Framework/Vulkan/Renderer.h"
#include "Framework/InputEvents/Event.h"
#include "Framework/Scene.h"
#include "Framework/Timestep.h"
#include "Framework/PhysicsSystem.h"

namespace Rava {
class Camera;
class Engine {
	friend class Editor;

   public:
	static Engine* s_Instance;
	glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.0f);
	u16 frameLimit       = 144;

	// static constexpr int WINDOW_WIDTH  = 1280;
	// static constexpr int WINDOW_HEIGHT = 720;

	enum class EngineState {
		Edit,
		Debug,
		Run,
	};
	EngineState engineState = EngineState::Edit;

   public:
	Engine();
	~Engine();

	NO_COPY(Engine)

	void Run();

	// void OnEvent(Event& event);
	void LoadScene(Unique<Scene> scene);
	float GetGamma() const { return m_gamma; }
	float GetExposure() const { return m_exposure; }
	GLFWwindow* GetGLFWWindow() { return m_ravaWindow.GetGLFWwindow(); }
	u32 GetCurrentFrameIndex() { return m_renderer.GetFrameIndex(); }
	PhysicsSystem& GetPhysicsSystem() { return m_physicsSystem; }
	physx::PxScene* GetCurrentPxScene() { return m_currentScene->GetPxScene(); }

   private:
	Log m_logger;
	std::string m_title = "Rava Engine";
	Window m_ravaWindow{m_title};
	static std::unique_ptr<Vulkan::Context> m_context;
	Vulkan::Renderer m_renderer{&m_ravaWindow};
	PhysicsSystem m_physicsSystem;
	Unique<Scene> m_currentScene = nullptr;

	float m_gamma    = 2.0f;
	float m_exposure = 1.0f;

	Timestep m_timestep{0ms};
	std::chrono::steady_clock::time_point m_timeLastFrame;
	std::chrono::steady_clock::time_point m_fpsLastUpdateTime;
	float m_targetFrameTime = 0.1f;
	u32 m_frameCount        = 0;

	static constexpr float PHYSICS_TIMESTEP = 1.0f / 60.0f;
	float m_accumulator                     = 0.0f;

	Camera m_mainCamera{};
	Camera m_editorCamera;
	glm::vec3 m_editorCameraPosition   = {0.0f, 2.0f, 2.0f};
	glm::vec3 m_editorCameraRotation   = {0.0f, 0.0f, 0.0f};
	glm::vec3 m_editorCameraForward    = {0.0f, 0.0f, -1.0f};
	glm::vec3 m_editorCameraRight      = {-m_editorCameraForward.z, 0.0f, m_editorCameraForward.x};
	glm::vec3 m_editorCameraUp         = {0.0f, 1.0f, 0.0f};
	glm::vec2 m_mouseRotateStartPos    = {0.0f, 0.0f};
	glm::vec2 m_mouseTranslateStartPos = {0.0f, 0.0f};

   private:
	void UpdateTitleFPS(std::chrono::steady_clock::time_point newTime);
	void EditorInputHandle();
	void RunButton();
	void UpdateEditorCamera();
	void UpdateSceneAndEntities();
	void UpdateSceneCamera();
	void UpdateRigidBodyTransform();
};
}  // namespace Rava
