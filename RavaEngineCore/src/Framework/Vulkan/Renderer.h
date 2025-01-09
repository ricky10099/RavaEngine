#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/SwapChain.h"
#include "Framework/Vulkan/RenderPass.h"
#include "Framework/Vulkan/Descriptor.h"
#include "Framework/Vulkan/Buffer.h"
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
	//void BeginFrame(Scene* scene);
	void EndFrame();
	//void BeginShadowRenderPass0(VkCommandBuffer commandBuffer);
	//void BeginShadowRenderPass1(VkCommandBuffer commandBuffer);
	void Begin3DRenderPass(/*VkCommandBuffer commandBuffer*/);
	//void BeginPostProcessingRenderPass(VkCommandBuffer commandBuffer);
	void BeginGUIRenderPass(VkCommandBuffer commandBuffer);
	void EndRenderPass(/*VkCommandBuffer commandBuffer*/);

	//void Render(Scene* scene);

	//void BeginFrame(Camera* camera);
	//void Renderpass3D(entt::registry& registry);
	//void SubmitShadows(entt::registry& registry, const std::vector<DirectionalLightComponent*>& directionalLights = {});
	//void Submit(Scene& scene) ;
	//void NextSubpass();
	//void LightingPass();
	//void PostProcessingRenderpass();
	//void TransparencyPass(entt::registry& registry, ParticleSystem* particleSystem);
	//void Submit2D(Camera* camera, entt::registry& registry);
	//void GUIRenderpass(Camera* camera);
	void EndScene();
	//void DrawWithTransform(const Sprite& sprite, const glm::mat4& transform) ;
	//void Draw(const Sprite& sprite, const glm::mat4& position, const glm::vec4& color, const float textureID = 1.0f);
	//void UpdateAnimations(entt::registry& registry, const Timestep& timestep) ;

	void SetAmbientLightIntensity(float ambientLightIntensity) {
		m_ambientLightIntensity = ambientLightIntensity;
	}
	void ShowDebugShadowMap(bool showDebugShadowMap)  { m_showDebugShadowMap = showDebugShadowMap; }
	//void ToggleDebugWindow(const GenericCallback& callback = nullptr) { m_Imgui = Imgui::ToggleDebugWindow(callback); }

	int GetFrameIndex() const;
	VkCommandBuffer GetCurrentCommandBuffer() const;
	std::shared_ptr<RenderPass> GetRenderPass() { return m_renderPass; }
	u32 GetImageCount() { return m_swapChain->ImageCount(); }
	float GetAmbientLightIntensity() const { return m_ambientLightIntensity; }
	u32 GetFrameCounter() const { return m_frameCounter; }
	float GetAspectRatio() const { return m_swapChain->ExtentAspectRatio(); }
	u32 GetContextWidth() const { return m_swapChain->Width(); }
	u32 GetContextHeight() const { return m_swapChain->Height(); }
	bool FrameInProgress() const { return m_frameInProgress; }

   private:
	enum ShadowMaps {
		HIGH_RES = 0,
		LOW_RES,
		NUMBER_OF_SHADOW_MAPS
	};

   private:
	bool m_shadersCompiled;
	Rava::Window* m_ravaWindow;
	std::unique_ptr<SwapChain> m_swapChain;

	std::shared_ptr<RenderPass> m_renderPass;
	// std::unique_ptr<VK_ShadowMap> m_ShadowMap[NUMBER_OF_SHADOW_MAPS];

	/*std::unique_ptr<VK_RenderSystemPbr> m_RenderSystemPbr;
	std::unique_ptr<VK_RenderSystemPbrSA> m_RenderSystemPbrSA;
	std::unique_ptr<VK_RenderSystemGrass> m_RenderSystemGrass;
	std::unique_ptr<VK_RenderSystemShadowInstanced> m_RenderSystemShadowInstanced;
	std::unique_ptr<VK_RenderSystemShadowAnimatedInstanced> m_RenderSystemShadowAnimatedInstanced;
	std::unique_ptr<VK_RenderSystemDeferredShading> m_RenderSystemDeferredShading;
	std::unique_ptr<VK_RenderSystemPostProcessing> m_RenderSystemPostProcessing;
	std::unique_ptr<VK_RenderSystemBloom> m_RenderSystemBloom;
	std::unique_ptr<VK_RenderSystemCubemap> m_RenderSystemCubemap;
	std::unique_ptr<VK_RenderSystemSpriteRenderer> m_RenderSystemSpriteRenderer;
	std::unique_ptr<VK_RenderSystemSpriteRenderer2D> m_RenderSystemSpriteRenderer2D;
	std::unique_ptr<VK_RenderSystemGUIRenderer> m_RenderSystemGUIRenderer;
	std::unique_ptr<VK_RenderSystemDebug> m_RenderSystemDebug;
	std::unique_ptr<VK_LightSystem> m_LightSystem;*/

	// std::shared_ptr<Imgui> m_Imgui;

	std::vector<VkCommandBuffer> m_commandBuffers;
	VkCommandBuffer m_currentCommandBuffer = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_globalDescriptorSetLayout = VK_NULL_HANDLE;

	u32 m_currentImageIndex;
	int m_currentFrameIndex;
	u32 m_frameCounter;
	bool m_frameInProgress;
	FrameInfo m_frameInfo{};

	// std::unique_ptr<DescriptorSetLayout> m_ShadowMapDescriptorSetLayout;
	// std::unique_ptr<DescriptorSetLayout> m_LightingDescriptorSetLayout;
	// std::unique_ptr<DescriptorSetLayout> m_PostProcessingDescriptorSetLayout;

	// std::vector<VkDescriptorSet> m_ShadowDescriptorSets0{VK_SwapChain::MAX_FRAMES_IN_FLIGHT};
	// std::vector<VkDescriptorSet> m_ShadowDescriptorSets1{VK_SwapChain::MAX_FRAMES_IN_FLIGHT};
	std::vector<VkDescriptorSet> m_globalDescriptorSets{MAX_FRAMES_SYNC};
	std::vector<VkDescriptorSet> m_localDescriptorSets{MAX_FRAMES_SYNC};
	std::vector<std::unique_ptr<Buffer>> m_uniformBuffers{MAX_FRAMES_SYNC};
	// std::vector<std::unique_ptr<VK_Buffer>> m_ShadowUniformBuffers0{VK_SwapChain::MAX_FRAMES_IN_FLIGHT};
	// std::vector<std::unique_ptr<VK_Buffer>> m_ShadowUniformBuffers1{VK_SwapChain::MAX_FRAMES_IN_FLIGHT};
	// std::vector<VkDescriptorSet> m_ShadowMapDescriptorSets{VK_SwapChain::MAX_FRAMES_IN_FLIGHT};
	// std::vector<VkDescriptorSet> m_LightingDescriptorSets{VK_SwapChain::MAX_FRAMES_IN_FLIGHT};
	// std::vector<VkDescriptorSet> m_PostProcessingDescriptorSets{VK_SwapChain::MAX_FRAMES_IN_FLIGHT};

	float m_ambientLightIntensity;
	glm::mat4 m_GUIViewProjectionMatrix{1.0f};

	bool m_showDebugShadowMap;

	private:
	//void CompileShaders();
	void CreateCommandBuffers();
	void FreeCommandBuffers();
	void RecreateSwapChain();
	void RecreateRenderpass();
	//void RecreateShadowMaps();
	//void CreateShadowMapDescriptorSets();
	//void CreateLightingDescriptorSets();
	//void CreatePostProcessingDescriptorSets();
	//void CreateRenderSystemBloom();
	void Recreate();
	//void UpdateTransformCache(Scene& scene, uint const nodeIndex, glm::mat4 const& parentMat4, bool parentDirtyFlag);
};
}  // namespace Vulkan