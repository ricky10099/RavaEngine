#include "ravapch.h"

#include "Framework/RavaEngine.h"

namespace Rava {
std::unique_ptr<Vulkan::Context> Engine::m_context;

Engine::Engine() {
	m_context = std::make_unique<Vulkan::Context>(&m_ravaWindow);
	m_renderer.Init();
	LoadScene(std::make_unique<Scene>());
}

Engine::~Engine() {
	ENGINE_INFO("Destruct Engine");
}

void Engine::Run() {
	while (!m_ravaWindow.ShouldClose()) {
		m_currentScene->Update();

		if (auto commandBuffer = m_renderer.BeginFrame()) {
			int frameIndex = m_renderer.GetFrameIndex();
			//FrameInfo frameInfo{
			//	frameIndex,
			//	frameTime,
			//	commandBuffer,
			//	globalDescriptorSets[frameIndex],
			//};

			//// update
			//GlobalUbo ubo{};
			//for (auto [entity, cam] : m_currentScene->m_entityRoot.view<Components::Camera>().each()) {
			//	if (cam.currentCamera) {
			//		ubo.projection  = cam.camera.GetProjection();
			//		ubo.view        = cam.camera.GetView();
			//		ubo.inverseView = cam.camera.GetInverseView();
			//	}
			//}
			//entityPointLightSystem.Update(frameInfo, ubo, m_currentScene->m_entityRoot);
			//uboBuffers[frameIndex]->WriteToBuffer(&ubo);
			//uboBuffers[frameIndex]->Flush();

			// render
			m_renderer.Begin3DRenderPass(commandBuffer);

			// order here matters
			//entityRenderSystem.RenderEntities(frameInfo, m_currentScene->m_entityRoot);
			//entityPointLightSystem.Render(frameInfo, m_currentScene->m_entityRoot);

			m_renderer.EndRenderPass(commandBuffer);
			m_renderer.EndFrame();
		}

		glfwPollEvents();
	}
}

void Engine::LoadScene(std::unique_ptr<Scene> scene) {
	m_currentScene = std::move(scene);
	ENGINE_INFO("Initializing {0}", typeid(scene.get()).name());
	m_currentScene->Init();
}
}  // namespace Rava
