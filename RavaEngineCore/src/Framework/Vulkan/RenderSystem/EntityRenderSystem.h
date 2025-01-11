#pragma once

#include "Framework/Vulkan/Pipeline.h"

namespace Vulkan {
class EntityRenderSystem {
   public:
	EntityRenderSystem(VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> globalSetLayouts);
	~EntityRenderSystem();

	NO_COPY(EntityRenderSystem)

	void Render(FrameInfo& frameInfo, entt::registry& registry);

   private:
	void CreatePipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout);
	void CreatePipeline(VkRenderPass renderPass);

	Unique<Pipeline> m_pipeline;
	VkPipelineLayout m_pipelineLayout;
};
}  // namespace Vulkan
