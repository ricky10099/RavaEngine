#include "ravapch.h"

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RenderSystem/WireframeRenderSystem.h"
#include "Framework/PhysicsSystem.h"
#include "Framework/Components.h"
#include "Framework/Timestep.h"
#include "Framework/Resources/MeshModel.h"

namespace Vulkan {
struct WireframePushConstants {
	glm::vec4 position{};
	glm::vec4 color{};
	float radius;
};

WireframeRenderSystem::WireframeRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) {
	CreatePipelineLayout(globalSetLayout);
	CreatePipeline(renderPass);
}

WireframeRenderSystem::~WireframeRenderSystem() {
	vkDestroyPipelineLayout(VKContext->GetLogicalDevice(), m_pipelineLayout, nullptr);
}

void WireframeRenderSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout) {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset     = 0;
	pushConstantRange.size       = sizeof(WireframePushConstants);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = static_cast<u32>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts            = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

	VkResult result = vkCreatePipelineLayout(VKContext->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
	VK_CHECK(result, "Failed to Create Pipeline Layout!");
}

void WireframeRenderSystem::CreatePipeline(VkRenderPass renderPass) {
	ENGINE_ASSERT(m_pipelineLayout != nullptr, "Cannot Create Pipeline before Pipeline Layout!");

	PipelineConfig pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);
	pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	// Pipeline::EnableAlphaBlending(pipelineConfig);
	pipelineConfig.attributeDescriptions.clear();
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.renderPass     = renderPass;
	pipelineConfig.pipelineLayout = m_pipelineLayout;
	m_pipeline = std::make_unique<Pipeline>("Shaders/Wireframe.vert.spv", "Shaders/Wireframe.frag.spv", pipelineConfig);
}

void WireframeRenderSystem::Update(FrameInfo& frameInfo, entt::registry& registry) {
}

void WireframeRenderSystem::Render(FrameInfo& frameInfo, entt::registry& registry) {
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

	auto view = registry.view<Rava::Component::RigidBody, Rava::Component::Transform>();
	for (auto entity : view) {
		auto& rigidBody = view.get<Rava::Component::RigidBody>(entity);
		auto& transform = view.get<Rava::Component::Transform>(entity);

		physx::PxShape* shape;
		rigidBody.actor->getShapes(&shape, 1);

		//const physx::PxGeometry* geometry = &shape->getGeometry();

		// Get wireframe vertices
		auto wireframeVertices = Rava::PhysicsSystem::CreateWireframeVertices(shape);

		// Update vertex buffer with wireframe data
		//UpdateVertexBuffer(wireframeVertices);
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			wireframeVertices.size() * sizeof(Rava::Vertex),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		// Copy vertex data to staging buffer
		void* data;
		vkMapMemory(VKContext->GetLogicalDevice(), stagingBufferMemory, 0, wireframeVertices.size() * sizeof(Rava::Vertex), 0, &data);
		memcpy(data, wireframeVertices.data(), wireframeVertices.size() * sizeof(Rava::Vertex));
		vkUnmapMemory(VKContext->GetLogicalDevice(), stagingBufferMemory);

		// Copy from staging buffer to vertex buffer
		CopyBuffer(stagingBuffer, vertexBuffer, vertices.size() * sizeof(Vertex));

		// Clean up staging buffer
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);


		m_pipeline->Bind(frameInfo.commandBuffer);
		// Bind wireframe pipeline
		//vkCmdBindPipeline(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Bind);

		// Draw lines
		vkCmdDraw(frameInfo.commandBuffer, wireframeVertices.size(), 1, 0, 0);
	}

	//auto view = registry.view<Rava::Component::PointLight, Rava::Component::Transform>();
	//for (auto entity : view) {
	//	auto& pointLight = view.get<Rava::Component::PointLight>(entity);
	//	auto& transform  = view.get<Rava::Component::Transform>(entity);

	//	WireframePushConstants push{};
	//	push.position = glm::vec4(transform.position, 1.f);
	//	push.color    = glm::vec4(pointLight.color, pointLight.lightIntensity);

	//	push.radius = pointLight.radius;

	//	vkCmdPushConstants(
	//		frameInfo.commandBuffer,
	//		m_pipelineLayout,
	//		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	//		0,
	//		sizeof(PointLightPushConstants),
	//		&push
	//	);
	//	vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
	//}
}
}  // namespace Vulkan
