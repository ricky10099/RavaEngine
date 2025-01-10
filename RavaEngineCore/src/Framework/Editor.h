#pragma once

namespace Rava {
class Editor {
   public:
	Editor(VkRenderPass renderPass, u32 imageCount);
	~Editor();

	NO_COPY(Editor)

	void NewFrame();
	void Render(VkCommandBuffer commandBuffer);
	void Run();

	void RecreateDescriptorSet(VkImageView swapChainImage, u32 imageCount);

   private:
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	VkSampler m_textureSampler = VK_NULL_HANDLE;

	bool m_viewportFocused = false, m_viewportHovered = false;

	glm::vec2 m_viewportSize = {0.0f, 0.0f};
	glm::vec2 m_viewportBounds[2];
};
}  // namespace Rava