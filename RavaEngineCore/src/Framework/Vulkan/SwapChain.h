#pragma once

#include "Framework/Vulkan/Context.h"

namespace Vulkan {
class SwapChain {
   public:
	SwapChain(VkExtent2D windowExtent);
	SwapChain(VkExtent2D windowExtent, std::shared_ptr<SwapChain> previous);
	~SwapChain();

	NO_COPY(SwapChain)

	VkResult AcquireNextImage(u32* imageIndex);
	VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, u32* imageIndex);
	bool CompareSwapFormats(const SwapChain& swapChain) const;
	void TransitionSwapChainImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, u32 currentImageIndex, VkCommandBuffer commandBuffer);

	VkImageView GetImageView(int index) { return m_swapChainImageViews[index]; }
	size_t ImageCount() { return m_swapChainImages.size(); }
	VkFormat GetSwapChainImageFormat() const { return m_swapChainImageFormat; }
	VkExtent2D GetSwapChainExtent() const { return m_swapChainExtent; }
	u32 Width() const { return m_swapChainExtent.width; }
	u32 Height() const { return m_swapChainExtent.height; }

	float ExtentAspectRatio() const {
		return static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height);
	}

   private:
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;

	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	std::shared_ptr<SwapChain> m_oldSwapChain;
	VkExtent2D m_windowExtent;

	VkSwapchainKHR m_swapChain;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;
	size_t m_currentFrame = 0;

   private:
	void Init();
	void CreateSwapChain();
	void CreateSwapChainImageViews();
	void CreateSyncObjects();

	// Helper functions
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
};
}  // namespace Vulkan