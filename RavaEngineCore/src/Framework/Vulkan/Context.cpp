#include "ravapch.h"

#include "Framework/Vulkan/Context.h"
#include "Framework/Vulkan/VKUtils.h"

namespace Vulkan {
Context* Context::m_context = nullptr;

Context::Context(Rava::Window* window)
	: m_ravaWindow(window) {
	if (m_context == nullptr) {
		m_context = this;
	} else {
		ENGINE_CRITICAL("Vulkan Context already exist!");
	}

	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateCommandPool();
}

Context::~Context() {
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	vkDestroyDevice(m_device, nullptr);

	if (ENABLE_VALIDATION) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void Context::CreateInstance() {
	if (ENABLE_VALIDATION && !CheckValidationLayerSupport()) {
		ENGINE_CRITICAL("Validation Layers requested, but not available!");
	}

	// Information about the application itself
	// Most data here doesn't affect the program and is for developer convenience
	VkApplicationInfo appInfo  = {};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "Rava Engine";             // Custom name of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // Custom version of the application
	appInfo.pEngineName        = "Rava Engine";             // Custom engine name
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);  // Custom engine version
	appInfo.apiVersion         = VK_API_VERSION_1_3;        // The Vulkan Version

	// Createion information for a VkInstance (Vulkan Instance)
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo     = &appInfo;

	// Check Instance Extensions supported...
	auto extensions = GetRequiredExtensions();
	HasGflwRequiredInstanceExtensions();

	createInfo.enabledExtensionCount   = static_cast<u32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (ENABLE_VALIDATION) {
		createInfo.enabledLayerCount   = static_cast<u32>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext             = nullptr;
	}

	// Create instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
	VK_CHECK(result, "Failed to Create a Vulkan Instance!")
}

void Context::SetupDebugMessenger() {
	if (!ENABLE_VALIDATION) {
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);
	VkResult result = CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);
	VK_CHECK(result, "Failed to Set Up Debug Messenger!")
}

void Context::CreateSurface() {
	// Create Surface (creates a surface create info struct, runs the create surface function)
	VkResult result = glfwCreateWindowSurface(m_instance, m_ravaWindow->GetGLFWwindow(), nullptr, &m_surface);
	VK_CHECK(result, "Failed to create a surface!");
}

void Context::PickPhysicalDevice() {
	// Enumerate Physical devices the vkInstance can access
	u32 deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	// If no device available, then none support Vulkan!
	if (deviceCount == 0) {
		ENGINE_CRITICAL("Failed to Find GPUs with Vulkan support!");
	}
	ENGINE_INFO("Device count: {0}", deviceCount);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (IsDeviceSuitable(device)) {
			m_physicalDevice = device;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE) {
		ENGINE_CRITICAL("Failed to Find a suitable GPU!");
	}

	// Information about the device itself (ID, name, type, vendor, etc)
	vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
	ENGINE_INFO("Physical Device: ", properties.deviceName);
}

void Context::CreateLogicalDevice() {
	// Get the queue family indices for the chosen Physical Device
	m_queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

	// Vector for queue creation information, and set for family indices
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<u32> uniqueQueueFamilies = {m_queueFamilyIndices.graphicsFamily, m_queueFamilyIndices.presentFamily};

	// Queue the logical device needs to create and info to do so(Only 1 for now, will add more later!)
	float queuePriority = 1.0f;
	for (u32 queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex        = queueFamily;  // The index of the family to create a queue from
		queueCreateInfo.queueCount              = 1;            // Number of queues to create
		// Vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Physical Device Features the Logical Device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy        = VK_TRUE;  // Enable Anisotropy

	// Information to create logical device (sometimes called "device")
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos    = queueCreateInfos.data();

	// Physical Device features Logical Device will use
	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = static_cast<u32>(DEVICE_EXTENSIONS.size());
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	// Create the logical device for the given physical devic
	VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
	VK_CHECK(result, "Failed to Create a Logical Device!")

	// Queues are created at the same time as the device...
	// So we want handle to queues
	// From given logical device, of given Queue Family, of given Queue Index(0 since only one queue), place reference in given
	// VkQueue
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily, 0, &m_presentQueue);
}

void Context::CreateCommandPool() {
	// Get indices of queue families from device
	QueueFamilyIndices queueFamilyIndices = m_queueFamilyIndices;

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex =
		queueFamilyIndices.graphicsFamily;  // Queue Family type that buffers from this command pool will use

	// Create a Graphics queue Family Command Pool
	VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
	VK_CHECK(result, "Failed to Create Command Pool!");
}

bool Context::CheckValidationLayerSupport() {
	u32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : VALIDATION_LAYERS) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> Context::GetRequiredExtensions() {
	// Set up extensions Instance will use
	u32 glfwExtensionCount = 0;   // GLFW may require multiple extensions
	const char** glfwExtensions;  // Extensions passed as array of cstrings, so need pointer (the array) to pointer(the cstring)

	// Get GLFW extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Create list to hold instance extensions
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	// If validation enabled, add extension to report validation debug info
	if (ENABLE_VALIDATION) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void Context::HasGflwRequiredInstanceExtensions() {
	// Need to get number of extensions to create array of correct size to hold extensions
	u32 extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	// Create a list of VkExtensionProperties using count
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Check if given extensions are in list of available extensions
	ENGINE_INFO("Available Extensions:");
	std::unordered_set<std::string_view> available;
	for (const auto& extension : extensions) {
		ENGINE_INFO("\t{0}", extension.extensionName);
		available.insert(extension.extensionName);
	}

	ENGINE_INFO("Required Extensions:");
	auto requiredExtensions = GetRequiredExtensions();
	for (const auto& required : requiredExtensions) {
		ENGINE_INFO("\t{0}", required);
		if (available.find(required) == available.end()) {
			ENGINE_CRITICAL("Missing required glfw extension");
		}
	}
}

bool Context::IsDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = FindQueueFamilies(device);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	// Information about what the device can do (geo shader, tess shader, wide line, etc)
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices Context::FindQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	u32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	// Go through each queue family and check if it has at least 1 of the required types of queue
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		// First check if queue family has at least 1 queue in that family (could have no queues)
		// Queue can be multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if has required
		// type
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily         = i;  // If queue family is valid, then get index
			indices.graphicsFamilyHasValue = true;
		}

		// Check if Queue Family supports prestation
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily         = i;
			indices.presentFamilyHasValue = true;
		}

		// Check if queue family indices are in a valid state, stop searching if so
		if (indices.IsComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

bool Context::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
	// Get device extension count;
	u32 extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// If no extensions found, return false
	if (extensionCount == 0) {
		return false;
	}

	// Populate list of extensions
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string_view> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
	// Check for extension
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails Context::QuerySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;
	// -- CAPABILITIES --
	// Get the surface capabilities for the given surface on the given physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	// -- FORMATS --
	u32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	// If formats returned, get list of formats
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	// -- PRESENTATION MODES --
	u32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	// If presentation modes returned, get list of presentation modes
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}
	return details;
}
}  // namespace Vulkan