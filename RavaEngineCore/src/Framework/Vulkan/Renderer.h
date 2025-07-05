#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/SwapChain.h"
#include "Framework/Vulkan/RenderPass.h"
#include "Framework/Vulkan/Descriptor.h"
#include "Framework/Vulkan/RenderSystem/PointLightRenderSystem.h"
#include "Framework/Vulkan/RenderSystem/EntityRenderSystem.h"
#include "Framework/Vulkan/RenderSystem/EntityAnimationRenderSystem.h"
#include "Framework/Vulkan/Buffer.h"
#include "Framework/Camera.h"
#include "Framework/Editor.h"
#include "Framework/Scene.h"


namespace Vulkan {
class Renderer {
   public:
	static std::unique_ptr<DescriptorPool> s_descriptorPool;

   public:
	Renderer(Rava::Window* window);
	~Renderer();

	NO_COPY(Renderer)

	void Init();
	void BeginFrame();
	void EndFrame();
	void Begin3DRenderPass();
	void BeginGUIRenderPass();
	void EndRenderPass() const;

	void ResetEditor();
	void UpdateEditor(Rava::Scene* scene);
	void UpdateAnimations(entt::registry& registry);

	void RenderpassEntities(entt::registry& registry, Rava::Camera& camera);
	void RenderEntities(Rava::Scene* scene);
	void RenderEnv(entt::registry& registry);
	void RenderpassGUI();
	void EndScene();

	int GetFrameIndex() const;
	VkCommandBuffer GetCurrentCommandBuffer() const;
	std::shared_ptr<RenderPass> GetRenderPass() { return m_renderPass; }
	u32 GetImageCount() { return static_cast<u32>(m_swapChain->ImageCount()); }
	u32 GetFrameCounter() const { return m_frameCounter; }
	float GetAspectRatio() const { return m_swapChain->ExtentAspectRatio(); }
	u32 GetContextWidth() const { return m_swapChain->Width(); }
	u32 GetContextHeight() const { return m_swapChain->Height(); }
	bool FrameInProgress() const { return m_frameInProgress; }

   private:
	Rava::Window* m_ravaWindow;
	std::unique_ptr<SwapChain> m_swapChain;

	std::shared_ptr<RenderPass> m_renderPass;
	std::unique_ptr<EntityRenderSystem> m_entityRenderSystem;
	std::unique_ptr<EntityAnimationRenderSystem> m_entityAnimationRenderSystem;
	std::unique_ptr<PointLightRenderSystem> m_pointLightRenderSystem;

	std::vector<VkCommandBuffer> m_commandBuffers;
	VkCommandBuffer m_currentCommandBuffer = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_globalDescriptorSetLayout = VK_NULL_HANDLE;

	u32 m_currentImageIndex;
	int m_currentFrameIndex;
	u32 m_frameCounter;
	bool m_frameInProgress;
	FrameInfo m_frameInfo{};

	std::vector<VkDescriptorSet> m_globalDescriptorSets{MAX_FRAMES_SYNC};
	std::vector<std::unique_ptr<Buffer>> m_uniformBuffers{MAX_FRAMES_SYNC};

	Unique<Rava::Editor> m_editor = nullptr;

	private:
	void CreateCommandBuffers();
	void FreeCommandBuffers();
	void RecreateSwapChain();
	void RecreateRenderpass();
	void Recreate();
};
}  // namespace Vulkan