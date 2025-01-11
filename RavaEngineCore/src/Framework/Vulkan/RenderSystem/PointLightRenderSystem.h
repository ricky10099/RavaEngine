#pragma once

#include "Framework/Vulkan/Pipeline.h"

namespace Vulkan {
class PointLightRenderSystem {
   public:
	PointLightRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~PointLightRenderSystem();

	NO_COPY(PointLightRenderSystem)

	void Update(FrameInfo& frameInfo, GlobalUbo& ubo, entt::registry& registry);
	void Render(FrameInfo& frameInfo, entt::registry& registry);

   private:
	void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void CreatePipeline(VkRenderPass renderPass);

	Unique<Pipeline> m_pipeline;
	VkPipelineLayout m_pipelineLayout;

	std::map<float, entt::entity> m_SortedLight;
};
}  // namespace Vulkan
