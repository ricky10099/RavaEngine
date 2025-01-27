#include "ravapch.h"

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RenderSystem/EntityAnimationRenderSystem.h"
#include "Framework/Resources/MeshModel.h"
#include "Framework/Components.h"

namespace Vulkan {
struct EntityPushConstantData {
	glm::mat4 modelMatrix{1.f};
	glm::mat4 normalMatrix{1.f};
};

EntityAnimationRenderSystem::EntityAnimationRenderSystem(
	VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& globalSetLayout
) {
	CreatePipelineLayout(globalSetLayout);
	CreatePipeline(renderPass);
}

EntityAnimationRenderSystem::~EntityAnimationRenderSystem() {
	vkDestroyPipelineLayout(VKContext->GetLogicalDevice(), m_pipelineLayout, nullptr);
}

void EntityAnimationRenderSystem::CreatePipelineLayout(std::vector<VkDescriptorSetLayout>& globalSetLayout) {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset     = 0;
	pushConstantRange.size       = sizeof(EntityPushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = static_cast<u32>(globalSetLayout.size());
	pipelineLayoutInfo.pSetLayouts            = globalSetLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

	VkResult result = vkCreatePipelineLayout(VKContext->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
	VK_CHECK(result, "Failed to Create Pipeline Layout!");
}

void EntityAnimationRenderSystem::CreatePipeline(VkRenderPass renderPass) {
	ENGINE_ASSERT(m_pipelineLayout != nullptr, "Cannot Create Pipeline before Pipeline Layout!");

	PipelineConfig pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);
	pipelineConfig.renderPass     = renderPass;
	pipelineConfig.pipelineLayout = m_pipelineLayout;
	m_pipeline = std::make_unique<Pipeline>("Shaders/ModelAnimation.vert.spv", "Shaders/Model.frag.spv", pipelineConfig);
}

void EntityAnimationRenderSystem::Render(FrameInfo& frameInfo, entt::registry& registry) {
	m_pipeline->Bind(frameInfo.commandBuffer);

	auto view2 = registry.view<Rava::Component::Model, Rava::Component::Transform, Rava::Component::Animation>();
	for (auto entity : view2) {
		auto& mesh      = view2.get<Rava::Component::Model>(entity);
		auto& transform = view2.get<Rava::Component::Transform>(entity);

		if (mesh.model == nullptr) {
			continue;
		}
		EntityPushConstantData push{};
		push.modelMatrix  = transform.GetTransform() * mesh.offset.GetTransform();
		push.normalMatrix = transform.NormalMatrix() * mesh.offset.NormalMatrix();

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			m_pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(EntityPushConstantData),
			&push
		);

		mesh.model.get()->Bind(frameInfo.commandBuffer);
		mesh.model.get()->Draw(frameInfo, m_pipelineLayout);
	}
}
}  // namespace Vulkan
