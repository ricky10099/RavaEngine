#include "ravapch.h"

#include "Framework/RavaEngine.h"

namespace Rava {
Engine* Engine::s_engine = nullptr;
std::unique_ptr<Vulkan::Context> Engine::m_context;

Engine::Engine() {
	if (s_engine == nullptr) {
		s_engine = this;
	} else {
		ENGINE_CRITICAL("Engine already running!");
	}

	m_context = std::make_unique<Vulkan::Context>(&m_ravaWindow);
	m_renderer.Init();
	//m_editor = std::make_unique<Editor>(m_renderer.GetRenderPass()->Get3DRenderPass(), m_renderer.GetImageCount());
	LoadScene(std::make_unique<Scene>());
}

Engine::~Engine() {
	ENGINE_INFO("Destruct Engine");
	vkDeviceWaitIdle(VKContext->GetLogicalDevice());
}

void Engine::Run() {
	while (!m_ravaWindow.ShouldClose()) {
		m_currentScene->Update();

		m_renderer.BeginFrame();
		// if (auto commandBuffer = m_renderer.BeginFrame()) {
		int frameIndex = m_renderer.GetFrameIndex();
		// FrameInfo frameInfo{
		//	frameIndex,
		//	frameTime,
		//	commandBuffer,
		//	globalDescriptorSets[frameIndex],
		// };

		//// update
		// GlobalUbo ubo{};
		// for (auto [entity, cam] : m_currentScene->m_entityRoot.view<Components::Camera>().each()) {
		//	if (cam.currentCamera) {
		//		ubo.projection  = cam.camera.GetProjection();
		//		ubo.view        = cam.camera.GetView();
		//		ubo.inverseView = cam.camera.GetInverseView();
		//	}
		// }
		// entityPointLightSystem.Update(frameInfo, ubo, m_currentScene->m_entityRoot);
		// uboBuffers[frameIndex]->WriteToBuffer(&ubo);
		// uboBuffers[frameIndex]->Flush();

		// render
		m_renderer.Begin3DRenderPass(/*commandBuffer*/);
		//m_renderer.BeginGUIRenderPass(/*commandBuffer*/);

		// order here matters
		// entityRenderSystem.RenderEntities(frameInfo, m_currentScene->m_entityRoot);
		// entityPointLightSystem.Render(frameInfo, m_currentScene->m_entityRoot);
		//m_editor->NewFrame();
		//m_editor->Run();
		//m_editor->Render(m_renderer.GetCurrentCommandBuffer());
		m_renderer.UpdateEditor();
		m_renderer.EndScene();
		//m_renderer.EndRenderPass(/*commandBuffer*/);
		//m_renderer.EndFrame();
		//}

		glfwPollEvents();
	}
}

void Engine::LoadScene(std::unique_ptr<Scene> scene) {
	m_currentScene = std::move(scene);
	ENGINE_INFO("Initializing {0}", typeid(scene.get()).name());
	m_currentScene->Init();
}
}  // namespace Rava
