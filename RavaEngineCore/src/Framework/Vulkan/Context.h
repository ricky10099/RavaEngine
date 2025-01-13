#pragma once

#include "Framework/Window.h"

namespace Vulkan {
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	u32 graphicsFamily;
	u32 presentFamily;
	bool graphicsFamilyHasValue = false;
	bool presentFamilyHasValue  = false;
	bool IsComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

class Context {
   public:
	VkPhysicalDeviceProperties properties;

   public:
	Context(Rava::Window* window);
	~Context();

	NO_COPY(Context)
	NO_MOVE(Context)

	static Context* Get() { return m_context; }
	VkInstance GetInstance() const { return m_instance; }
	VkSurfaceKHR GetSurface() const { return m_surface; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
	VkDevice GetLogicalDevice() const { return m_device; }
	VkCommandPool GetCommandPool() const { return m_commandPool; }
	VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
	VkQueue GetPresentQueue() const { return m_presentQueue; }
	QueueFamilyIndices&  GetPhysicalQueueFamilies() { return m_queueFamilyIndices; }
	SwapChainSupportDetails GetSwapChainSupport() { return QuerySwapChainSupport(m_physicalDevice); }

   private:
	static Context* m_context;
	Rava::Window* m_ravaWindow;

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkCommandPool m_commandPool;

	QueueFamilyIndices m_queueFamilyIndices;
	VkDevice m_device;
	VkSurfaceKHR m_surface;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

   private:
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateCommandPool();

	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions();
	void HasGflwRequiredInstanceExtensions();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
};
}  // namespace Vulkan