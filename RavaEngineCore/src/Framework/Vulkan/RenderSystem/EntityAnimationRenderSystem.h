#pragma once

#include "Framework/Vulkan/Pipeline.h"

namespace Vulkan {
class EntityAnimationRenderSystem {
   public:
	EntityAnimationRenderSystem(VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& globalSetLayouts);
	~EntityAnimationRenderSystem();

	NO_COPY(EntityAnimationRenderSystem)

	void Render(FrameInfo& frameInfo, entt::registry& registry);

   private:
	void CreatePipelineLayout(std::vector<VkDescriptorSetLayout>& globalSetLayout);
	void CreatePipeline(VkRenderPass renderPass);

	Unique<Pipeline> m_pipeline;
	VkPipelineLayout m_pipelineLayout;
};
}  // namespace Vulkan
