#include "ravapch.h"

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/Pipeline.h"

namespace Vulkan {
Pipeline::Pipeline(std::string_view vertFilePath, std::string_view fragFilePath, const PipelineConfig& config) {
	CreateGraphicsPipeline(vertFilePath, fragFilePath, config);
}

Pipeline::~Pipeline() {
	vkDestroyShaderModule(VKContext->GetLogicalDevice(), m_vertModule, nullptr);
	vkDestroyShaderModule(VKContext->GetLogicalDevice(), m_fragModule, nullptr);
	vkDestroyPipeline(VKContext->GetLogicalDevice(), m_graphicsPipeline, nullptr);
}

void Pipeline::CreateGraphicsPipeline(
	std::string_view vertFilepath, std::string_view fragFilepath, const PipelineConfig& config
) {
	ENGINE_ASSERT(
		config.pipelineLayout != VK_NULL_HANDLE, "Cannot Create a Graphics Pipeline: No PipelineLayout Provided in ConfigInfo!"
	);
	ENGINE_ASSERT(
		config.renderPass != VK_NULL_HANDLE, "Cannot Create a Graphics Pipeline: No RenderPass Provided in ConfigInfo!"
	);

	auto vertCode = ReadShaderFromAssets(vertFilepath.data());
	auto fragCode = ReadShaderFromAssets(fragFilepath.data());

	CreateShaderModule(vertCode, &m_vertModule);
	CreateShaderModule(fragCode, &m_fragModule);

	VkPipelineShaderStageCreateInfo shaderStages[2];
	shaderStages[0].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage               = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module              = m_vertModule;
	shaderStages[0].pName               = "main";
	shaderStages[0].flags               = 0;
	shaderStages[0].pNext               = nullptr;
	shaderStages[0].pSpecializationInfo = nullptr;
	shaderStages[1].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module              = m_fragModule;
	shaderStages[1].pName               = "main";
	shaderStages[1].flags               = 0;
	shaderStages[1].pNext               = nullptr;
	shaderStages[1].pSpecializationInfo = nullptr;

	auto& bindingDescriptions   = config.bindingDescriptions;
	auto& attributeDescriptions = config.attributeDescriptions;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
	vertexInputInfo.vertexBindingDescriptionCount   = static_cast<u32>(bindingDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();
	vertexInputInfo.pVertexBindingDescriptions      = bindingDescriptions.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = shaderStages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &config.inputAssemblyInfo;
	pipelineInfo.pViewportState      = &config.viewportInfo;
	pipelineInfo.pRasterizationState = &config.rasterizationInfo;
	pipelineInfo.pMultisampleState   = &config.multisampleInfo;
	pipelineInfo.pColorBlendState    = &config.colorBlendInfo;
	pipelineInfo.pDepthStencilState  = &config.depthStencilInfo;
	pipelineInfo.pDynamicState       = &config.dynamicStateInfo;

	pipelineInfo.layout     = config.pipelineLayout;
	pipelineInfo.renderPass = config.renderPass;
	pipelineInfo.subpass    = config.subpass;

	pipelineInfo.basePipelineIndex  = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkResult result =
		vkCreateGraphicsPipelines(VKContext->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
	VK_CHECK(result, "Failed to Create Graphics Pipeline!");
}

void Pipeline::Bind(VkCommandBuffer commandBuffer) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
}
}  // namespace Vulkan