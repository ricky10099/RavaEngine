#include "ravapch.h"

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RenderSystem/PointLightRenderSystem.h"
#include "Framework/Components.h"
#include "Framework/Timestep.h"

namespace Vulkan {
struct PointLightPushConstants {
	glm::vec4 position{};
	glm::vec4 color{};
	float radius;
};

PointLightRenderSystem::PointLightRenderSystem(VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) {
	CreatePipelineLayout(globalSetLayout);
	CreatePipeline(renderPass);
}

PointLightRenderSystem::~PointLightRenderSystem() {
	vkDestroyPipelineLayout(VKContext->GetLogicalDevice(), m_pipelineLayout, nullptr);
}

void PointLightRenderSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout) {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset     = 0;
	pushConstantRange.size       = sizeof(PointLightPushConstants);

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

void PointLightRenderSystem::CreatePipeline(VkRenderPass renderPass) {
	ENGINE_ASSERT(m_pipelineLayout != nullptr, "Cannot Create Pipeline before Pipeline Layout!");

	PipelineConfig pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);
	// Pipeline::EnableAlphaBlending(pipelineConfig);
	pipelineConfig.attributeDescriptions.clear();
	pipelineConfig.bindingDescriptions.clear();
	pipelineConfig.renderPass     = renderPass;
	pipelineConfig.pipelineLayout = m_pipelineLayout;
	m_pipeline = std::make_unique<Pipeline>("Shaders/PointLight.vert.spv", "Shaders/PointLight.frag.spv", pipelineConfig);
}

void PointLightRenderSystem::Update(FrameInfo& frameInfo, GlobalUbo& ubo, entt::registry& registry) {
	// m_SortedLight.clear();
	// glm::vec3 cameraPosition;
	// for (auto [entity, cam, transform] : registry.view<Rava::Component::Camera, Rava::Component::Transform>().each()) {
	//	if (cam.currentCamera) {
	//		cameraPosition = transform.position;
	//	}
	// }

	// int lightIndex   = 0;
	// auto view      = registry.view<Rava::Component::PointLight, Rava::Component::Transform>();
	// for (auto entity : view) {
	//	auto& transform = view.get<Rava::Component::Transform>(entity);

	//	ENGINE_ASSERT(lightIndex < MAX_LIGHTS);

	//	auto mat4Global     = transform.GetTransform();
	//	constexpr int column = 3;
	//	auto lightPosition   = glm::vec3(mat4Global[column][0], mat4Global[column][1], mat4Global[column][2]);
	//	auto distanceVec     = cameraPosition - lightPosition;
	//	float distanceToCam  = glm::dot(distanceVec, distanceVec);

	//	m_SortedLight.insert({distanceToCam, entity});

	//	lightIndex++;
	//}
	// std::map<float, entt::entity>::reverse_iterator it;
	// lightIndex = 0;
	// for (it = m_SortedLight.rbegin(); it != m_SortedLight.rend(); it++) {
	//	auto entity      = it->second;
	//	auto& transform  = view.get<Rava::Component::Transform>(entity);
	//	auto& pointLight = view.get<Rava::Component::PointLight>(entity);

	//	// copy light to ubo
	//	auto mat4Global                         = transform.GetTransform();
	//	constexpr int column                     = 3;
	//	auto lightPosition                       = glm::vec3(mat4Global[column][0], mat4Global[column][1], mat4Global[column][2]);
	//	ubo.pointLights[lightIndex].position = glm::vec4(lightPosition, 0.0f);
	//	ubo.pointLights[lightIndex].color    = glm::vec4(pointLight.color, pointLight.lightIntensity);

	//	lightIndex++;
	//}
	// ubo.numPointLights = lightIndex;

	// Point light
	{
		int lightIndex = 0;

		auto view = registry.view<Rava::Component::PointLight, Rava::Component::Transform>();
		for (auto entity : view) {
			auto& pointLight = view.get<Rava::Component::PointLight>(entity);
			auto& transform  = view.get<Rava::Component::Transform>(entity);

			assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

			// copy light to ubo
			ubo.pointLights[lightIndex].position = glm::vec4(transform.position, 1.0f);
			ubo.pointLights[lightIndex].color    = glm::vec4(pointLight.color, pointLight.lightIntensity);

			lightIndex += 1;
		}
		ubo.numPointLights = lightIndex;
	}

	// Directional light
	{
		int lightIndex = 0;
		auto view      = registry.view<Rava::Component::DirectionalLight>();
		for (auto entity : view) {
			auto& directionalLight = view.get<Rava::Component::DirectionalLight>(entity);

			ASSERT(lightIndex < MAX_LIGHTS);

			// copy light to ubo
			ubo.directionalLight.direction = glm::vec4(directionalLight.direction, 0.0f);
			ubo.directionalLight.color     = glm::vec4(directionalLight.color, directionalLight.lightIntensity);

			lightIndex++;
		}
	}
	//ubo.m_NumberOfActiveDirectionalLights = lightIndex;
}

void PointLightRenderSystem::Render(FrameInfo& frameInfo, entt::registry& registry) {
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

	auto view = registry.view<Rava::Component::PointLight, Rava::Component::Transform>();
	for (auto entity : view) {
		auto& pointLight = view.get<Rava::Component::PointLight>(entity);
		auto& transform  = view.get<Rava::Component::Transform>(entity);

		PointLightPushConstants push{};
		push.position = glm::vec4(transform.position, 1.f);
		push.color    = glm::vec4(pointLight.color, pointLight.lightIntensity);

		push.radius = pointLight.radius;

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			m_pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PointLightPushConstants),
			&push
		);
		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
	}
}
}  // namespace Vulkan
