#include "ravapch.h"

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/SwapChain.h"

namespace Vulkan {
SwapChain::SwapChain(VkExtent2D extent)
	: m_windowExtent(extent) {
	Init();
}

SwapChain::SwapChain(VkExtent2D extent, std::shared_ptr<SwapChain> previous)
	: m_windowExtent(extent)
	, m_oldSwapChain(previous) {
	Init();
	m_oldSwapChain.reset();
}

void SwapChain::Init() {
	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateSyncObjects();
}

SwapChain::~SwapChain() {
	for (auto imageView : m_swapChainImageViews) {
		vkDestroyImageView(VKContext->GetLogicalDevice(), imageView, nullptr);
	}

	if (m_swapChain != nullptr) {
		vkDestroySwapchainKHR(VKContext->GetLogicalDevice(), m_swapChain, nullptr);
		m_swapChain = nullptr;
	}

	// cleanup synchronization objects
	for (size_t i = 0; i < MAX_FRAMES_SYNC; i++) {
		vkDestroySemaphore(VKContext->GetLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(VKContext->GetLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(VKContext->GetLogicalDevice(), m_inFlightFences[i], nullptr);
	}
}

void SwapChain::CreateSwapChain() {
	// Get Swap Chain details so we can pick best settings
	SwapChainSupportDetails m_SwapChainSupport = VKContext->GetSwapChainSupport();

	// Find optimal surface values for our swap chain
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(m_SwapChainSupport.formats);
	VkPresentModeKHR presentMode     = ChooseSwapPresentMode(m_SwapChainSupport.presentModes);
	VkExtent2D extent                = ChooseSwapExtent(m_SwapChainSupport.capabilities);

	// How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
	// If imageCount higher than max, then clamp down to max
	// if 0, then limitless
	u32 imageCount = m_SwapChainSupport.capabilities.minImageCount + 1;
	if (m_SwapChainSupport.capabilities.maxImageCount > 0 && imageCount > m_SwapChainSupport.capabilities.maxImageCount) {
		imageCount = m_SwapChainSupport.capabilities.maxImageCount;
	}

	// Creation information for swap chain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface                  = VKContext->GetSurface();               // Swapchain surface
	createInfo.minImageCount            = imageCount;                            // Minimum images in swapchain
	createInfo.imageFormat              = surfaceFormat.format;                  // Swapchain format
	createInfo.imageColorSpace          = surfaceFormat.colorSpace;              // Swapchain colour space
	createInfo.imageExtent              = extent;                                // Swapchain image extents
	createInfo.imageArrayLayers         = 1;                                     // Number of layers for each image in chain
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	;                                                                            // What attachment images will be used as
	createInfo.preTransform = m_SwapChainSupport.capabilities.currentTransform;  // Transform to perform on swap chain images
	createInfo.compositeAlpha =
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;  // How to handle blending images with external graphics(e.g. other windows)
	createInfo.presentMode = presentMode;   // Swapchain presentation mode
	createInfo.clipped     = VK_TRUE;  // Whether to clip parts of image not in view (e.g. behind another window, off screen, etc)

	// Get Queue Family Indices
	QueueFamilyIndices& indices = VKContext->GetPhysicalQueueFamilies();
	u32 queueFamilyIndices[]    = {static_cast<u32>(indices.graphicsFamily), static_cast<u32>(indices.presentFamily)};

	// If Graphics and Presentation families are different, then swapchain must let images be shared between families
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;  // Image share handling
		createInfo.queueFamilyIndexCount = 2;                           // Numberof queues to share images between
		createInfo.pQueueFamilyIndices   = queueFamilyIndices;          // Array of queues to share between
	} else {
		createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;        // Optional
		createInfo.pQueueFamilyIndices   = nullptr;  // Optional
	}

	// If old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
	createInfo.oldSwapchain = (m_oldSwapChain == nullptr ? VK_NULL_HANDLE : m_oldSwapChain->m_swapChain);

	// Create Swapchain
	VkResult result = vkCreateSwapchainKHR(VKContext->GetLogicalDevice(), &createInfo, nullptr, &m_swapChain);
	VK_CHECK(result, "Failed to create swap chain!");

	// we only specified a minimum number of images in the swap chain, so the implementation is
	// allowed to create a swap chain with more. That's why we'll first query the final number of
	// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
	// retrieve the handles.
	vkGetSwapchainImagesKHR(VKContext->GetLogicalDevice(), m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(VKContext->GetLogicalDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

	// Store for later reference
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent      = extent;
}

void SwapChain::CreateSwapChainImageViews() {
	m_swapChainImageViews.resize(m_swapChainImages.size());
	for (size_t i = 0; i < m_swapChainImages.size(); i++) {

		CreateImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_swapChainImageViews[i]);
	}
}

void SwapChain::CreateSyncObjects() {
	m_imageAvailableSemaphores.resize(MAX_FRAMES_SYNC);
	m_renderFinishedSemaphores.resize(MAX_FRAMES_SYNC);
	m_inFlightFences.resize(MAX_FRAMES_SYNC);
	m_imagesInFlight.resize(ImageCount(), VK_NULL_HANDLE);

	// Semaphore create information
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence create information
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_SYNC; i++) {
		if (vkCreateSemaphore(VKContext->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i])
				!= VK_SUCCESS
			|| vkCreateSemaphore(VKContext->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i])
				   != VK_SUCCESS
			|| vkCreateFence(VKContext->GetLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
			ENGINE_CRITICAL("Failed to create Synchronization objects for a frame!");
		}
	}
}

// Best format is subjective, but ours will be:
// Format		 : VK_FORMAT_B8G8R8A8_SRGB (VK_FORMAT_R8G8B8A8_SRGB as backup)
// colorSpace	 : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	// Search for format
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
			|| availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB
				   && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	// If can't find optimal format, then just return first format
	return availableFormats[0];
}

VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	// Look for Mailbox presentation mode
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			ENGINE_INFO("Present mode: Mailbox");
			return availablePresentMode;
		}
	}

	// for (const auto &availablePresentMode : availablePresentModes) {
	//   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
	//     ENGINE_INFO("Present mode: Immediate");
	//     return availablePresentMode;
	//   }
	// }

	ENGINE_INFO("Present mode: V-Sync");
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
	// If current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window.
	if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
		return capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = m_windowExtent;
		actualExtent.width =
			std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height =
			std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkResult SwapChain::AcquireNextImage(u32* imageIndex) {
	// -- GET NEXT IMAGE --
	// Wait for given fence to signal (open) from last draw before continuing
	vkWaitForFences(
		VKContext->GetLogicalDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<u64>::max()
	);

	VkResult result = vkAcquireNextImageKHR(
		VKContext->GetLogicalDevice(),
		m_swapChain,
		std::numeric_limits<u64>::max(),
		m_imageAvailableSemaphores[m_currentFrame],  // must be a not signaled semaphore
		VK_NULL_HANDLE,
		imageIndex
	);

	return result;
}

VkResult SwapChain::SubmitCommandBuffers(const VkCommandBuffer* buffers, u32* imageIndex) {
	if (m_imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(VKContext->GetLogicalDevice(), 1, &m_imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
	}
	m_imagesInFlight[*imageIndex] = m_inFlightFences[m_currentFrame];

	// Manually reset (close) fence
	vkResetFences(VKContext->GetLogicalDevice(), 1, &m_inFlightFences[m_currentFrame]);

	// -- SUBMIT COMAND BUFFER TO RENDER --
	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[]      = {m_imageAvailableSemaphores[m_currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount     = 1;               // Number of semaphores to wait on
	submitInfo.pWaitSemaphores        = waitSemaphores;  // List of semaphores to wait on
	submitInfo.pWaitDstStageMask      = waitStages;      // Stages to check semaphores at
	submitInfo.commandBufferCount     = 1;               // Number of command buffers to submit
	submitInfo.pCommandBuffers        = buffers;         // Command buffer to submit

	VkSemaphore signalSemaphores[]  = {m_renderFinishedSemaphores[m_currentFrame]};
	submitInfo.signalSemaphoreCount = 1;                 // Number of semaphores to signal
	submitInfo.pSignalSemaphores    = signalSemaphores;  // Semaphores to signal when command buffer finishes

	// Submit command buffer to queue
	VkResult result = vkQueueSubmit(VKContext->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]);
	VK_CHECK(result, "failed to submit draw command buffer!");

	// -- PRESENT RENDERED IMAGE TO SCREEN --
	VkPresentInfoKHR presentInfo   = {};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;                 // Number of semaphores to wait on
	presentInfo.pWaitSemaphores    = signalSemaphores;  // Semaphores to wait on
	VkSwapchainKHR m_SwapChains[]  = {m_swapChain};
	presentInfo.swapchainCount     = 1;             // Number of swapchains to present to
	presentInfo.pSwapchains        = m_SwapChains;  // Swapchains to present image to
	presentInfo.pImageIndices      = imageIndex;    // Index of images in swapchains to present

	// Present image
	result = vkQueuePresentKHR(VKContext->GetPresentQueue(), &presentInfo);

	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_SYNC;

	return result;
}

bool SwapChain::CompareSwapFormats(const SwapChain& swapChain) const {
	bool imageFormatEqual = (swapChain.m_swapChainImageFormat == m_swapChainImageFormat);
	return (imageFormatEqual);
}

void SwapChain::TransitionSwapChainImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, u32 currentImageIndex, VkCommandBuffer commandBuffer) {
	//Vulkan::TransitionImageLayout(m_swapChainImages[m_currentFrame], oldLayout, newLayout);

	VkImageMemoryBarrier imageMemoryBarrier            = {};
	imageMemoryBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout                       = oldLayout;                // Layout to transition from
	imageMemoryBarrier.newLayout                       = newLayout;                // Layout to transition to
	imageMemoryBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;  // Queue family to transition from
	imageMemoryBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;  // Queue family to transition to
	imageMemoryBarrier.image = m_swapChainImages[currentImageIndex];  // Image being accessed and modified as part of barrier
	imageMemoryBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;  // Aspect of image being altered
	imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;                          // First mip level to start alterations on
	imageMemoryBarrier.subresourceRange.levelCount     = 1;  // Number of mip levels to alter starting from baseMipLevel
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;          // First layer to start alterations on
	imageMemoryBarrier.subresourceRange.layerCount     = 1;          // Number of layers to alter stating from baseArrayLayer

	VkPipelineStageFlags srcStage{};
	VkPipelineStageFlags dstStage{};

	// If transitioning from color attachment to shader readable...
	if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	// If transitioning from shader read only to present source...
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	} else {
		ENGINE_ERROR("Unsupported Layout Transition in Swap Chain!");
		return;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage,
		dstStage,  // Pipeline stages(match to src and dst AccessMasks)
		0,         // Dependency flags
		0,
		nullptr,  // Memory Barrier count + data
		0,
		nullptr,  // Buffer Memory Barrier count + data
		1,
		&imageMemoryBarrier  // Image Memory Barrier count + data
	);
}

}  // namespace Vulkan