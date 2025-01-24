#pragma once

#include "Framework/Vulkan/Pipeline.h"

struct FrameInfo;
namespace Vulkan {
class WireframeRenderSystem {
   public:
	WireframeRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~WireframeRenderSystem();

	NO_COPY(WireframeRenderSystem)

	void Update(FrameInfo& frameInfo, entt::registry& registry);
	void Render(FrameInfo& frameInfo, entt::registry& registry);

   private:
	void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void CreatePipeline(VkRenderPass renderPass);

	Unique<Pipeline> m_pipeline;
	VkPipelineLayout m_pipelineLayout;
};
}  // namespace Vulkan
