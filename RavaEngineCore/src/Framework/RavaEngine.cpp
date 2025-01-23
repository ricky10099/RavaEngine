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
}

void Engine::Run() {
	if (m_currentScene == nullptr) {
		LoadScene(std::make_unique<Scene>("Scene"));
	}

	m_timeLastFrame = std::chrono::high_resolution_clock::now();
	m_physicsSystem.Pause();
	while (!m_ravaWindow.ShouldClose()) {
		if (frameLimit != 0) {
			m_targetFrameTime = 1.0f / frameLimit;
		}
		auto newTime    = std::chrono::high_resolution_clock::now();
		m_timestep      = newTime - m_timeLastFrame;
		m_timeLastFrame = newTime;

		switch (engineState) {
			case EngineState::Run:
				m_physicsSystem.Update(m_currentScene.get(), m_timestep);
				UpdateSceneCamera();
				UpdateSceneAndEntities();
				//UpdateRigidBodyTransform();
				RunButton();
				break;
			case EngineState::Debug:
				m_physicsSystem.Update(m_currentScene.get(), m_timestep);
				UpdateSceneAndEntities();
				[[fallthrough]];
			case EngineState::Edit:
				EditorInputHandle();
				UpdateEditorCamera();
				break;
		}

		m_renderer.BeginFrame();
		m_renderer.UpdateEditor(m_currentScene.get());
		UpdateRigidBodyTransform();
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
		m_physicsSystem.DisconnectPVD();
		m_currentScene->ClearScene();
		m_currentScene.reset();
	}
	m_currentScene = std::move(scene);
	ENGINE_INFO("Initializing {0}", m_currentScene->GetName());
	m_currentScene->CreatePhysXScene();
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
	static glm::vec3 currentRotation{0.0f};
	if (Input::IsMouseButtonDown(Mouse::Button1)) {
		m_mouseRotateStartPos = Input::GetMousePosition();
		currentRotation       = m_editorCameraRotation;
	}

	if (Input::IsMouseButtonPress(Mouse::Button1)) {
		glm::vec3 moveDirection{0.0f};
		glm::vec3 rotation{0.0f};
		
		if (Input::IsKeyPress(Key::W)) {
			moveDirection += m_editorCameraForward;
		}
		if (Input::IsKeyPress(Key::S)) {
			moveDirection -= m_editorCameraForward;
		}
		if (Input::IsKeyPress(Key::A)) {
			moveDirection -= m_editorCameraRight;
		}
		if (Input::IsKeyPress(Key::D)) {
			moveDirection += m_editorCameraRight;
		}
		if (Input::IsKeyPress(Key::Q)) {
			moveDirection += m_editorCameraUp;
		}
		if (Input::IsKeyPress(Key::E)) {
			moveDirection -= m_editorCameraUp;
		}

		if (Input::IsMouseButtonDown()) {
			m_mouseTranslateStartPos = Input::GetMousePosition();
		}

		if (Input::IsMouseButtonPress()) {
			if (Input::GetMouseY() - m_mouseTranslateStartPos.y > 100.0f) {
				moveDirection -= m_editorCameraForward;
			} else if (Input::GetMouseY() - m_mouseTranslateStartPos.y < -100.0f) {
				moveDirection += m_editorCameraForward;
			}

			if (Input::GetMouseX() - m_mouseTranslateStartPos.x > 100.0f) {
				moveDirection += m_editorCameraUp;
			} else if (Input::GetMouseX() - m_mouseTranslateStartPos.x < -100.0f) {
				moveDirection -= m_editorCameraUp;
			}
		} else {
			rotation.y = (Input::GetMouseX() - m_mouseRotateStartPos.x) / m_ravaWindow.Width();
			rotation.x = (Input::GetMouseY() - m_mouseRotateStartPos.y) / m_ravaWindow.Height();
		}

		if (glm::dot(rotation, rotation) > std::numeric_limits<float>::epsilon()) {
			m_editorCameraRotation = currentRotation - 1.0f * rotation;
		}
		if (glm::dot(moveDirection, moveDirection) > std::numeric_limits<float>::epsilon()) {
			m_editorCameraPosition += 3.0f * Timestep::Count() * glm::normalize(moveDirection);
		}
	}

	if (Input::IsKeyDown(Key::F5)) {
		if (engineState == EngineState::Edit) {
			engineState = EngineState::Debug;
			m_physicsSystem.Unpause();
		} else if (engineState == EngineState::Debug) {
			engineState = EngineState::Edit;
			m_physicsSystem.Pause();
			m_physicsSystem.DisconnectPVD();
			vkDeviceWaitIdle(VKContext->GetLogicalDevice());
			m_currentScene->ClearScene();
			auto scene = std::move(m_currentScene);
			LoadScene(std::move(scene));
		}
	}

	if (Input::IsKeyDown(Key::F7)) {
		if (m_physicsSystem.IsPaused()) {
			m_physicsSystem.Unpause();
		} else {
			m_physicsSystem.Pause();
		}
	}

	RunButton();
}

void Engine::RunButton() {
	if (Input::IsKeyDown(Key::F6)) {
		if (engineState == EngineState::Edit) {
			engineState = EngineState::Run;
			m_physicsSystem.Unpause();
		} else if (engineState == EngineState::Run) {
			engineState = EngineState::Edit;
			m_physicsSystem.Pause();
			m_physicsSystem.DisconnectPVD();
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
	m_editorCameraForward = {-sin(m_editorCameraRotation.y), 0.0f, -cos(m_editorCameraRotation.y)};
	m_editorCameraRight   = {-m_editorCameraForward.z, 0.0f, m_editorCameraForward.x};
	m_editorCameraUp      = {0.0f, 1.0f, 0.0f};
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

void Engine::UpdateRigidBodyTransform() {
	for (auto [entity, rigidBody, transform] : m_currentScene->GetRegistry().view<Component::RigidBody, Component::Transform>().each()) {
		glm::vec3 eulerAngles(
			glm::radians(transform.rotation.x), glm::degrees(transform.rotation.y), glm::radians(transform.rotation.z)
		);
		glm::quat rotation   = glm::quat(eulerAngles);
		physx::PxQuat pxRotation(rotation.x, rotation.y, rotation.z, rotation.w);
		physx::PxVec3 pxPosition(transform.position.x, transform.position.y, transform.position.z);
		//physx::PxTransform t = ToTransform(transform.position, transform.GetQuaternion());
		physx::PxTransform t(pxPosition, pxRotation);
		rigidBody.actor->setGlobalPose(t);
	}
}
}  // namespace Rava
