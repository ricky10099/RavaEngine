#pragma once

namespace Vulkan {
struct PipelineConfig {
	PipelineConfig() = default;

	NO_COPY(PipelineConfig)

	std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass         = nullptr;
	u32 subpass                     = 0;
};

class Pipeline {
   public:
	Pipeline(std::string_view vertFilePath, std::string_view fragFilePath, const PipelineConfig& config);
	~Pipeline();

	NO_COPY(Pipeline)

	void Bind(VkCommandBuffer commandBuffer);

	static void DefaultPipelineConfig(PipelineConfig& config);
	static void EnableAlphaBlending(PipelineConfig& config);

	private:
	void CreateGraphicsPipeline(std::string_view vertFilePath, std::string_view fragFilePath, const PipelineConfig& config);

	void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

	VkPipeline m_graphicsPipeline;
	VkShaderModule m_vertModule;
	VkShaderModule m_fragModule;
};
}  // namespace Vulkan