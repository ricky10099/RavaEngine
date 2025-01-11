#include "ravapch.h"

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RenderSystem/EntityRenderSystem.h"
#include "Framework/Resources/MeshModel.h"
#include "Framework/Components.h"

namespace Vulkan {
struct EntityPushConstantData {
	glm::mat4 modelMatrix{1.f};
	glm::mat4 normalMatrix{1.f};
};

EntityRenderSystem::EntityRenderSystem(VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> globalSetLayout) {
	CreatePipelineLayout(globalSetLayout);
	CreatePipeline(renderPass);
}

EntityRenderSystem::~EntityRenderSystem() {
	vkDestroyPipelineLayout(VKContext->GetLogicalDevice(), m_pipelineLayout, nullptr);
}

void EntityRenderSystem::CreatePipelineLayout(std::vector<VkDescriptorSetLayout> globalSetLayout) {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset     = 0;
	pushConstantRange.size       = sizeof(EntityPushConstantData);

	// std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = static_cast<u32>(globalSetLayout.size());
	pipelineLayoutInfo.pSetLayouts            = globalSetLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

	VkResult result = vkCreatePipelineLayout(VKContext->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
	VK_CHECK(result, "Failed to Create Pipeline Layout!");
}

void EntityRenderSystem::CreatePipeline(VkRenderPass renderPass) {
	ENGINE_ASSERT(m_pipelineLayout != nullptr, "Cannot Create Pipeline before Pipeline Layout!");

	PipelineConfig pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);
	pipelineConfig.renderPass     = renderPass;
	pipelineConfig.pipelineLayout = m_pipelineLayout;
	m_pipeline = std::make_unique<Pipeline>("Shaders/Model.vert.spv", "Shaders/Model.frag.spv", pipelineConfig);
}

void EntityRenderSystem::Render(FrameInfo& frameInfo, entt::registry& registry) {
	m_pipeline->Bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineLayout,
		0,
		1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr
	);

	auto view2 = registry.view<Rava::Component::Model, Rava::Component::Transform>();
	for (auto entity : view2) {
		auto& mesh      = view2.get<Rava::Component::Model>(entity);
		auto& transform = view2.get<Rava::Component::Transform>(entity);

		if (mesh.model == nullptr) {
			continue;
		}
		EntityPushConstantData push{};
		push.modelMatrix  = mesh.offset.GetTransform() * transform.GetTransform();
		push.normalMatrix = mesh.offset.NormalMatrix() * transform.NormalMatrix();

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			m_pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(EntityPushConstantData),
			&push
		);

		static_cast<Rava::MeshModel*>(mesh.model.get())->Bind(frameInfo, m_pipelineLayout);
		static_cast<Rava::MeshModel*>(mesh.model.get())->Draw(frameInfo, m_pipelineLayout);
	}
}
}  // namespace Vulkan
