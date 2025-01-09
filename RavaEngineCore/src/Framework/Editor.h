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

   private:
	VkDescriptorPool m_descriptorPool;
};
}  // namespace Rava