#include "ravapch.h"

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/RenderPass.h"

namespace Vulkan {
RenderPass::RenderPass(SwapChain* swapChain)
	: m_renderPassExtent{swapChain->GetSwapChainExtent()}
	, m_swapChain{swapChain} {
	m_depthFormat = FindDepthFormat();
	// m_BufferPositionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	// m_BufferNormalFormat   = VK_FORMAT_R16G16B16A16_SFLOAT;
	// m_BufferColorFormat    = VK_FORMAT_R8G8B8A8_UNORM;
	// m_BufferMaterialFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	// m_BufferEmissionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

	Create3DRenderPass();
	// CreatePostProcessingRenderPass();
	CreateGUIRenderPass();

	CreateColorAttachmentResources();
	CreateDepthResources();

	// CreateGBufferImages();
	// CreateGBufferImageViews();

	Create3DFramebuffers();
	// CreatePostProcessingFramebuffers();
	CreateGUIFramebuffers();
}

RenderPass::~RenderPass() {
	vkDestroyImageView(VKContext->GetLogicalDevice(), m_depthImageView, nullptr);
	vkDestroyImage(VKContext->GetLogicalDevice(), m_depthImage, nullptr);
	vkFreeMemory(VKContext->GetLogicalDevice(), m_depthImageMemory, nullptr);

	vkDestroyImageView(VKContext->GetLogicalDevice(), m_colorAttachmentView, nullptr);
	vkDestroyImage(VKContext->GetLogicalDevice(), m_colorAttachmentImage, nullptr);
	vkFreeMemory(VKContext->GetLogicalDevice(), m_colorAttachmentImageMemory, nullptr);

	for (auto framebuffer : m_3DFramebuffers) {
		vkDestroyFramebuffer(VKContext->GetLogicalDevice(), framebuffer, nullptr);
	}
	//  for (auto framebuffer : m_PostProcessingFramebuffers) {
	//	vkDestroyFramebuffer(VKContext->GetLogicalDevice(), framebuffer, nullptr);
	//  }
	for (auto framebuffer : m_GUIFramebuffers) {
		vkDestroyFramebuffer(VKContext->GetLogicalDevice(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(VKContext->GetLogicalDevice(), m_3DRenderPass, nullptr);
	//  vkDestroyRenderPass(VKContext->GetLogicalDevice(), m_PostProcessingRenderPass, nullptr);
	vkDestroyRenderPass(VKContext->GetLogicalDevice(), m_GUIRenderPass, nullptr);
}

void RenderPass::Create3DRenderPass() {
	// ATTACHMENT_COLOR
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format                  = m_swapChain->GetSwapChainImageFormat();
	colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment            = static_cast<u32>(RenderTargets3D::ATTACHMENT_COLOR);
	colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// ATTACHMENT_DEPTH
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format         = FindDepthFormat();
	depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = static_cast<u32>(RenderTargets3D::ATTACHMENT_DEPTH);
	depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

#pragma region GBufferAttachment
	//// ATTACHMENT_GBUFFER_POSITION
	// VkAttachmentDescription gBufferPositionAttachment = {};
	// gBufferPositionAttachment.format                  = m_BufferPositionFormat;
	// gBufferPositionAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	// gBufferPositionAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// gBufferPositionAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	// gBufferPositionAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// gBufferPositionAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// gBufferPositionAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	// gBufferPositionAttachment.finalLayout             = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// VkAttachmentReference gBufferPositionAttachmentRef = {};
	// gBufferPositionAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_POSITION);
	// gBufferPositionAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// VkAttachmentReference gBufferPositionInputAttachmentRef = {};
	// gBufferPositionInputAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_POSITION);
	// gBufferPositionInputAttachmentRef.layout                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	//// ATTACHMENT_GBUFFER_NORMAL
	// VkAttachmentDescription gBufferNormalAttachment = {};
	// gBufferNormalAttachment.format                  = m_BufferNormalFormat;
	// gBufferNormalAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	// gBufferNormalAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// gBufferNormalAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	// gBufferNormalAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// gBufferNormalAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// gBufferNormalAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	// gBufferNormalAttachment.finalLayout             = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// VkAttachmentReference gBufferNormalAttachmentRef = {};
	// gBufferNormalAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_NORMAL);
	// gBufferNormalAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// VkAttachmentReference gBufferNormalInputAttachmentRef = {};
	// gBufferNormalInputAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_NORMAL);
	// gBufferNormalInputAttachmentRef.layout                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	//// ATTACHMENT_GBUFFER_COLOR
	// VkAttachmentDescription gBufferColorAttachment = {};
	// gBufferColorAttachment.format                  = m_BufferColorFormat;
	// gBufferColorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	// gBufferColorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// gBufferColorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	// gBufferColorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// gBufferColorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// gBufferColorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	// gBufferColorAttachment.finalLayout             = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// VkAttachmentReference gBufferColorAttachmentRef = {};
	// gBufferColorAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_COLOR);
	// gBufferColorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// VkAttachmentReference gBufferColorInputAttachmentRef = {};
	// gBufferColorInputAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_COLOR);
	// gBufferColorInputAttachmentRef.layout                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	//// ATTACHMENT_GBUFFER_MATERIAL
	// VkAttachmentDescription gBufferMaterialAttachment = {};
	// gBufferMaterialAttachment.format                  = VK_FORMAT_R16G16B16A16_SFLOAT;
	// gBufferMaterialAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	// gBufferMaterialAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// gBufferMaterialAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	// gBufferMaterialAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// gBufferMaterialAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// gBufferMaterialAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	// gBufferMaterialAttachment.finalLayout             = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// VkAttachmentReference gBufferMaterialAttachmentRef = {};
	// gBufferMaterialAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_MATERIAL);
	// gBufferMaterialAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// VkAttachmentReference gBufferMaterialInputAttachmentRef = {};
	// gBufferMaterialInputAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_MATERIAL);
	// gBufferMaterialInputAttachmentRef.layout                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	//// ATTACHMENT_GBUFFER_EMISSION
	// VkAttachmentDescription gBufferEmissionAttachment = {};
	// gBufferEmissionAttachment.format                  = m_BufferEmissionFormat;
	// gBufferEmissionAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	// gBufferEmissionAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// gBufferEmissionAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	// gBufferEmissionAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// gBufferEmissionAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	// gBufferEmissionAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	// gBufferEmissionAttachment.finalLayout             = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// VkAttachmentReference gBufferEmissionAttachmentRef = {};
	// gBufferEmissionAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_EMISSION);
	// gBufferEmissionAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// VkAttachmentReference gBufferEmissionInputAttachmentRef = {};
	// gBufferEmissionInputAttachmentRef.attachment            = static_cast<uint>(RenderTargets3D::ATTACHMENT_GBUFFER_EMISSION);
	// gBufferEmissionInputAttachmentRef.layout                = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	//// geometry pass
	// std::array<VkAttachmentReference, NUMBER_OF_GBUFFER_ATTACHMENTS> gBufferAttachments = {
	//	gBufferPositionAttachmentRef, gBufferNormalAttachmentRef, gBufferColorAttachmentRef, gBufferMaterialAttachmentRef,
	//	gBufferEmissionAttachmentRef
	// };

	// VkSubpassDescription subpassGeometry    = {};
	// subpassGeometry.flags                   = 0;
	// subpassGeometry.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	// subpassGeometry.inputAttachmentCount    = 0;
	// subpassGeometry.pInputAttachments       = nullptr;
	// subpassGeometry.colorAttachmentCount    = NUMBER_OF_GBUFFER_ATTACHMENTS;
	// subpassGeometry.pColorAttachments       = gBufferAttachments.data();
	// subpassGeometry.pResolveAttachments     = nullptr;
	// subpassGeometry.pDepthStencilAttachment = &depthAttachmentRef;
	// subpassGeometry.preserveAttachmentCount = 0;
	// subpassGeometry.pPreserveAttachments    = nullptr;

	//// lighting pass
	// std::array<VkAttachmentReference, NUMBER_OF_GBUFFER_ATTACHMENTS> inputAttachments = {
	//	gBufferPositionInputAttachmentRef, gBufferNormalInputAttachmentRef, gBufferColorInputAttachmentRef,
	//	gBufferMaterialInputAttachmentRef, gBufferEmissionInputAttachmentRef
	// };

	// VkSubpassDescription subpassLighting    = {};
	// subpassLighting.flags                   = 0;
	// subpassLighting.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	// subpassLighting.inputAttachmentCount    = NUMBER_OF_GBUFFER_ATTACHMENTS;
	// subpassLighting.pInputAttachments       = inputAttachments.data();
	// subpassLighting.colorAttachmentCount    = 1;
	// subpassLighting.pColorAttachments       = &colorAttachmentRef;
	// subpassLighting.pResolveAttachments     = nullptr;
	// subpassLighting.pDepthStencilAttachment = &depthAttachmentRef;
	// subpassLighting.preserveAttachmentCount = 0;
	// subpassLighting.pPreserveAttachments    = nullptr;
#pragma endregion

	// transparency pass
	VkSubpassDescription subpassTransparency    = {};
	subpassTransparency.flags                   = 0;
	subpassTransparency.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassTransparency.inputAttachmentCount    = 0;
	subpassTransparency.pInputAttachments       = nullptr;
	subpassTransparency.colorAttachmentCount    = 1;
	subpassTransparency.pColorAttachments       = &colorAttachmentRef;
	subpassTransparency.pResolveAttachments     = nullptr;
	subpassTransparency.pDepthStencilAttachment = &depthAttachmentRef;
	subpassTransparency.preserveAttachmentCount = 0;
	subpassTransparency.pPreserveAttachments    = nullptr;

	VkSubpassDependency dependency = {};
	dependency.dstSubpass          = 0;
	dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
	dependency.srcAccessMask       = 0;
	dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo              = {};
	renderPassInfo.sType                               = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount                     = static_cast<u32>(attachments.size());
	renderPassInfo.pAttachments                        = attachments.data();
	renderPassInfo.subpassCount                        = 1;
	renderPassInfo.pSubpasses                          = &subpassTransparency;
	renderPassInfo.dependencyCount                     = 1;
	renderPassInfo.pDependencies                       = &dependency;

#pragma region GBufferDependencies
	// constexpr u32 NUMBER_OF_DEPENDENCIES = 4;
	// std::array<VkSubpassDependency, NUMBER_OF_DEPENDENCIES> dependencies;

	//// lighting depends on geometry
	// dependencies[0].srcSubpass =
	//	static_cast<u32>(SubPasses3D::SUBPASS_GEOMETRY);  // Index of the render pass being depended upon by dstSubpass
	// dependencies[0].dstSubpass =
	//	static_cast<u32>(SubPasses3D::SUBPASS_LIGHTING);  // The index of the render pass depending on srcSubpass
	// dependencies[0].srcStageMask =
	//	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // What pipeline stage must have completed for the dependency
	// dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;  // What pipeline stage is waiting on the dependency
	// dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
	//							  | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;   // What access scopes influence the dependency
	// dependencies[0].dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;  // What access scopes are waiting on the dependency
	// dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;          // Other configuration about the dependency

	//// transparency depends on lighting
	// dependencies[1].srcSubpass      = static_cast<u32>(SubPasses3D::SUBPASS_LIGHTING);
	// dependencies[1].dstSubpass      = static_cast<u32>(SubPasses3D::SUBPASS_TRANSPARENCY);
	// dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	// dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	// dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// dependencies[2].srcSubpass      = VK_SUBPASS_EXTERNAL;
	// dependencies[2].dstSubpass      = static_cast<u32>(SubPasses3D::SUBPASS_GEOMETRY);
	// dependencies[2].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	// dependencies[2].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// dependencies[2].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	// dependencies[2].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// dependencies[3].srcSubpass      = static_cast<u32>(SubPasses3D::SUBPASS_GEOMETRY);
	// dependencies[3].dstSubpass      = VK_SUBPASS_EXTERNAL;
	// dependencies[3].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// dependencies[3].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	// dependencies[3].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// dependencies[3].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	// dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	//// render pass
	// std::array<VkAttachmentDescription, static_cast<u32>(RenderTargets3D::NUMBER_OF_ATTACHMENTS)> attachments = {
	//	colorAttachment,        depthAttachment,           gBufferPositionAttachment, gBufferNormalAttachment,
	//	gBufferColorAttachment, gBufferMaterialAttachment, gBufferEmissionAttachment
	// };
	// std::array<VkSubpassDescription, static_cast<uint>(SubPasses3D::NUMBER_OF_SUBPASSES)> subpasses = {
	//	subpassGeometry, subpassLighting, subpassTransparency
	// };

	// VkRenderPassCreateInfo renderPassInfo = {};
	// renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	// renderPassInfo.attachmentCount        = static_cast<u32>(RenderTargets3D::NUMBER_OF_ATTACHMENTS);
	// renderPassInfo.pAttachments           = attachments.data();
	// renderPassInfo.subpassCount           = static_cast<u32>(SubPasses3D::NUMBER_OF_SUBPASSES);
	// renderPassInfo.pSubpasses             = subpasses.data();
	// renderPassInfo.dependencyCount        = NUMBER_OF_DEPENDENCIES;
	// renderPassInfo.pDependencies          = dependencies.data();
#pragma endregion

	VkResult result = vkCreateRenderPass(VKContext->GetLogicalDevice(), &renderPassInfo, nullptr, &m_3DRenderPass);
	VK_CHECK(result, "Failed to create a 3D Render Pass!");
}

void RenderPass::CreateGUIRenderPass() {
	// ATTACHMENT_COLOR
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format                  = m_swapChain->GetSwapChainImageFormat();
	colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
	// colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment            = static_cast<u32>(RenderTargetsGUI::ATTACHMENT_COLOR);
	colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// subpass
	VkSubpassDescription subpassGUI    = {};
	subpassGUI.flags                   = 0;
	subpassGUI.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassGUI.inputAttachmentCount    = 0;
	subpassGUI.pInputAttachments       = nullptr;
	subpassGUI.colorAttachmentCount    = 1;
	subpassGUI.pColorAttachments       = &colorAttachmentRef;
	subpassGUI.pResolveAttachments     = nullptr;
	subpassGUI.pDepthStencilAttachment = nullptr;
	subpassGUI.preserveAttachmentCount = 0;
	subpassGUI.pPreserveAttachments    = nullptr;

	constexpr u32 NUMBER_OF_DEPENDENCIES = 2;
	std::array<VkSubpassDependency, NUMBER_OF_DEPENDENCIES> dependencies{};

	dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass      = static_cast<u32>(SubPassesGUI::SUBPASS_GUI);
	dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass      = static_cast<u32>(SubPassesGUI::SUBPASS_GUI);
	dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount        = static_cast<u32>(RenderTargetsGUI::NUMBER_OF_ATTACHMENTS);
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = static_cast<u32>(SubPassesGUI::NUMBER_OF_SUBPASSES);
	renderPassInfo.pSubpasses      = &subpassGUI;
	renderPassInfo.dependencyCount = NUMBER_OF_DEPENDENCIES;
	renderPassInfo.pDependencies = dependencies.data();

	VkResult result = vkCreateRenderPass(VKContext->GetLogicalDevice(), &renderPassInfo, nullptr, &m_GUIRenderPass);
	VK_CHECK(result, "Failed to create a GUI Render Pass!");
}

void RenderPass::CreateColorAttachmentResources() {
	VkFormat format = m_swapChain->GetSwapChainImageFormat();

	CreateImage(
		m_renderPassExtent.width,
		m_renderPassExtent.height,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_colorAttachmentImageMemory,
		m_colorAttachmentImage
	);

	CreateImageView(m_colorAttachmentImage, format, VK_IMAGE_ASPECT_COLOR_BIT, m_colorAttachmentView);
}

void RenderPass::CreateDepthResources() {
	m_depthFormat = FindDepthFormat();

	CreateImage(
		m_renderPassExtent.width,
		m_renderPassExtent.height,
		m_depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_depthImageMemory,
		m_depthImage
	);

	CreateImageView(m_depthImage, m_depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_depthImageView);
}

void RenderPass::Create3DFramebuffers() {
	// Resize framebuffer count to equal swap chain image count
	m_3DFramebuffers.resize(m_swapChain->ImageCount());

	// Create a framebuffer for each swap chain image
	for (size_t i = 0; i < m_swapChain->ImageCount(); i++) {
		std::array<VkImageView, static_cast<u32>(RenderTargets3D::NUMBER_OF_ATTACHMENTS)> attachments = {
			m_swapChain->GetImageView(static_cast<u32>(i)),
			// m_colorAttachmentView,
			m_depthImageView,
			// m_GBufferPositionView,
			// m_GBufferNormalView,
			// m_GBufferColorView,
			// m_GBufferMaterialView,
			// m_GBufferEmissionView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass              = m_3DRenderPass;  // Render Pass layout the Framebuffer will be used with
		framebufferInfo.attachmentCount         = static_cast<u32>(RenderTargets3D::NUMBER_OF_ATTACHMENTS);
		framebufferInfo.pAttachments            = attachments.data();         // List of attachments(1:1 with Render Pass)
		framebufferInfo.width                   = m_renderPassExtent.width;   // Framebuffer width
		framebufferInfo.height                  = m_renderPassExtent.height;  // FRamebuffer height
		framebufferInfo.layers                  = 1;                          // Framebuffer layers

		VkResult result = vkCreateFramebuffer(VKContext->GetLogicalDevice(), &framebufferInfo, nullptr, &m_3DFramebuffers[i]);
		VK_CHECK(result, "Failed to create a Framebuffer!")
	}
}

void RenderPass::CreateGUIFramebuffers() {
	m_GUIFramebuffers.resize(m_swapChain->ImageCount());
	for (size_t i = 0; i < m_swapChain->ImageCount(); i++) {
		std::array<VkImageView, static_cast<u32>(RenderTargetsGUI::NUMBER_OF_ATTACHMENTS)> attachments = {
			m_swapChain->GetImageView(static_cast<u32>(i))
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass              = m_GUIRenderPass;
		framebufferInfo.attachmentCount         = static_cast<u32>(RenderTargetsGUI::NUMBER_OF_ATTACHMENTS);
		framebufferInfo.pAttachments            = attachments.data();
		framebufferInfo.width                   = m_renderPassExtent.width;
		framebufferInfo.height                  = m_renderPassExtent.height;
		framebufferInfo.layers                  = 1;

		VkResult result = vkCreateFramebuffer(VKContext->GetLogicalDevice(), &framebufferInfo, nullptr, &m_GUIFramebuffers[i]);
		VK_CHECK(result, "Failed to create a Framebuffer!")
	}
}
}  // namespace Vulkan