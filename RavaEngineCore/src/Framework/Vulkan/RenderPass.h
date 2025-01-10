#pragma once

// #include "Framework/Vulkan/Context.h"
#include "Framework/Vulkan/SwapChain.h"

namespace Vulkan {
class RenderPass {
   public:
	enum class SubPasses3D {
		SUBPASS_GEOMETRY = 0,
		SUBPASS_LIGHTING,
		SUBPASS_TRANSPARENCY,
		NUMBER_OF_SUBPASSES,
	};

	enum class RenderTargets3D {
		ATTACHMENT_COLOR = 0,
		ATTACHMENT_DEPTH,
		//ATTACHMENT_GBUFFER_POSITION,
		//ATTACHMENT_GBUFFER_NORMAL,
		//ATTACHMENT_GBUFFER_COLOR,
		//ATTACHMENT_GBUFFER_MATERIAL,
		//ATTACHMENT_GBUFFER_EMISSION,
		NUMBER_OF_ATTACHMENTS,
	};

	enum class SubPassesPostProcessing {
		SUBPASS_BLOOM = 0,
		NUMBER_OF_SUBPASSES,
	};

	enum class RenderTargetsPostProcessing {
		ATTACHMENT_COLOR = 0,
		INPUT_ATTACHMENT_3DPASS_COLOR,
		INPUT_ATTACHMENT_GBUFFER_EMISSION,
		NUMBER_OF_ATTACHMENTS,
	};

	enum class SubPassesGUI {
		SUBPASS_GUI = 0,
		NUMBER_OF_SUBPASSES,
	};

	enum class RenderTargetsGUI {
		ATTACHMENT_COLOR = 0,
		NUMBER_OF_ATTACHMENTS,
	};

   public:
	RenderPass(SwapChain* swapChain);
	~RenderPass();

	NO_COPY(RenderPass)

	VkImageView GetImageViewColorAttachment() const { return m_colorAttachmentView; }
	// VkImageView GetImageViewGBufferPosition() { return m_GBufferPositionView; }
	// VkImageView GetImageViewGBufferNormal() { return m_GBufferNormalView; }
	// VkImageView GetImageViewGBufferColor() { return m_GBufferColorView; }
	// VkImageView GetImageViewGBufferMaterial() { return m_GBufferMaterialView; }
	// VkImageView GetImageViewGBufferEmission() { return m_GBufferEmissionView; }
	// VkImage GetImageEmission() const { return m_GBufferEmissionImage; }
	// VkFormat GetFormatEmission() const { return m_bufferEmissionFormat; }

	VkFramebuffer Get3DFrameBuffer(int index) { return m_3DFramebuffers[index]; }
	//VkFramebuffer GetGUIFrameBuffer(int index) { return m_GUIFramebuffers[index]; }
	// VkFramebuffer GetPostProcessingFrameBuffer(int index) { return m_PostProcessingFramebuffers[index]; }

	VkRenderPass Get3DRenderPass() const { return m_3DRenderPass; }
	//VkRenderPass GetGUIRenderPass() const { return m_GUIRenderPass; }
	// VkRenderPass GetPostProcessingRenderPass() { return m_PostProcessingRenderPass; }

   private:
	SwapChain* m_swapChain;
	VkExtent2D m_renderPassExtent;

	VkFormat m_depthFormat;
	// VkFormat m_bufferPositionFormat;
	// VkFormat m_bufferNormalFormat;
	// VkFormat m_bufferColorFormat;
	// VkFormat m_bufferMaterialFormat;
	// VkFormat m_bufferEmissionFormat;

	VkImage m_depthImage;
	VkImage m_colorAttachmentImage;
	// VkImage m_GBufferPositionImage;
	// VkImage m_GBufferNormalImage;
	// VkImage m_GBufferColorImage;
	// VkImage m_GBufferMaterialImage;
	// VkImage m_GBufferEmissionImage;

	VkImageView m_depthImageView;
	VkImageView m_colorAttachmentView;
	// VkImageView m_GBufferPositionView;
	// VkImageView m_GBufferNormalView;
	// VkImageView m_GBufferColorView;
	// VkImageView m_GBufferMaterialView;
	// VkImageView m_GBufferEmissionView;

	VkDeviceMemory m_depthImageMemory;
	VkDeviceMemory m_colorAttachmentImageMemory;
	// VkDeviceMemory m_GBufferPositionImageMemory;
	// VkDeviceMemory m_GBufferNormalImageMemory;
	// VkDeviceMemory m_GBufferColorImageMemory;
	// VkDeviceMemory m_GBufferMaterialImageMemory;
	// VkDeviceMemory m_GBufferEmissionImageMemory;

	std::vector<VkFramebuffer> m_3DFramebuffers;
	//std::vector<VkFramebuffer> m_GUIFramebuffers;
	// std::vector<VkFramebuffer> m_postProcessingFramebuffers;

	VkRenderPass m_3DRenderPass;
	//VkRenderPass m_GUIRenderPass;
	// VkRenderPass m_postProcessingRenderPass;

   private:
	void CreateColorAttachmentResources();
	void CreateDepthResources();

	void Create3DRenderPass();
	// void CreatePostProcessingRenderPass();
	//void CreateGUIRenderPass();

	void Create3DFramebuffers();
	// void CreatePostProcessingFramebuffers();
	//void CreateGUIFramebuffers();

	// void CreateGBufferImages();
	// void CreateGBufferImageViews();
	// void DestroyGBuffers();
};
}  // namespace Vulkan