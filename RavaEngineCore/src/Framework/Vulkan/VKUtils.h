#pragma once

#include "Framework/Vulkan/GPUSharedDefines.h"
#include "Framework/Vulkan/Context.h"

#define VK_CHECK(x, msg)      \
	if (x != VK_SUCCESS) {    \
		ENGINE_CRITICAL(msg); \
	}

static void CheckVKResult(VkResult err) {
	std::string_view msg("[vulkan] Error: VkResult = {0}", err);
	VK_CHECK(err, msg);
}

#define VKContext Vulkan::Context::Get()

//////////////////////////////////////////////////////////////////////////
// Vulkan config
//////////////////////////////////////////////////////////////////////////

static constexpr int MAX_FRAMES_SYNC = 2;

const std::vector<const char*> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME};

//////////////////////////////////////////////////////////////////////////
// GPU data transfer
//////////////////////////////////////////////////////////////////////////

struct PointLight {
	glm::vec4 position{};  // ignore w
	glm::vec4 color{};     // w is intensity
};

struct DirectionalLight {
	glm::vec4 direction{};  // ignore w
	glm::vec4 color{};      // w is intensity
};

struct GlobalUbo {
	glm::mat4 projection{1.f};
	glm::mat4 view{1.f};
	glm::mat4 inverseView{1.f};
	glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 0.02f};  // w is intensity
	PointLight pointLights[MAX_LIGHTS];
	DirectionalLight directionalLight;
	int numPointLights;
};

struct FrameInfo {
	int frameIndex;
	float frameTime;
	VkCommandBuffer commandBuffer;
	VkDescriptorSet globalDescriptorSet;
};

//////////////////////////////////////////////////////////////////////////
// Vulkan Function
//////////////////////////////////////////////////////////////////////////
namespace Vulkan {
static VkFormat FindSupportedFormat(
	const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features
) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(VKContext->GetPhysicalDevice(), format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	ENGINE_CRITICAL("Failed to Find Supported Format!");
	return {};
}

static VkFormat FindDepthFormat() {
	return FindSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

static u32 FindMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(VKContext->GetPhysicalDevice(), &memProperties);
	for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
		// index of memory type must match corresponding bit is allowedTypes
		if ((typeFilter & (1 << i))
			// Desired property bit flags are part of memory type's property flags
			&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			// This memory type is valid, so return index
			return i;
		}
	}

	ENGINE_CRITICAL("Failed to Find Suitable Memory Type!");
	return 0;
}

static void CreateBuffer(
	VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory
) {
	// CREATE VERTEX BUFFER
	// Information to create a buffer(doesn't include assigning memory)
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size        = size;                       // Size of buffer (size of 1 vertex * number of vertices)
	bufferInfo.usage       = usage;                      // Multiple types of buffer possible
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // Similar to Swap Chain images, can share vertex buffers

	VkResult result = vkCreateBuffer(VKContext->GetLogicalDevice(), &bufferInfo, nullptr, &buffer);
	VK_CHECK(result, "Failed to Create Vertex Buffer!");

	// GET BUFFER MEMORY REQUIREMENTS
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VKContext->GetLogicalDevice(), buffer, &memRequirements);

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	// Index of memory type on Physical Device that has required bit flags
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	result = vkAllocateMemory(VKContext->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory);
	VK_CHECK(result, "Failed to Allocate Vertex Buffer Memory!");

	vkBindBufferMemory(VKContext->GetLogicalDevice(), buffer, bufferMemory, 0);
}

static VkCommandBuffer BeginSingleTimeCommands() {
	// Command Buffer details
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool        = Vulkan::Context::Get()->GetCommandPool();
	allocInfo.commandBufferCount = 1;

	// Command buffer to hold transfer commands
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(VKContext->GetLogicalDevice(), &allocInfo, &commandBuffer);

	// Information to begin command buffer record
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Begin recording transfer commands
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

static void EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
	// End commands
	vkEndCommandBuffer(commandBuffer);

	// Queue submission information
	VkSubmitInfo submitInfo{};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &commandBuffer;

	// Submit transfer command to transfer queue and wait until finishes
	vkQueueSubmit(VKContext->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(VKContext->GetGraphicsQueue());

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(VKContext->GetLogicalDevice(), VKContext->GetCommandPool(), 1, &commandBuffer);
}

static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	// Create buffer
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	// Region of data to copy from and to
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;  // Optional
	copyRegion.dstOffset = 0;  // Optional
	copyRegion.size      = size;

	// Command to copy src buffer to dst buffer
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

static void CopyBufferToImage(VkBuffer buffer, VkImage image, u32 width, u32 height, u32 layerCount) {
	// Create buffer
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset      = 0;  // Offset into data
	region.bufferRowLength   = 0;  // Row length of data to calculate data spacing
	region.bufferImageHeight = 0;  // Image height to calculate data spacing

	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;  // Which aspect of image to copy
	region.imageSubresource.mipLevel       = 0;                          // Mipmap level to copy
	region.imageSubresource.baseArrayLayer = 0;                          // Starting array layer (if array)
	region.imageSubresource.layerCount     = layerCount;                 // Number of layers to copy starting at baseArrayLayer

	region.imageOffset = {0, 0, 0};           // Offset into image (as opposed to raw data in bufferOffset)
	region.imageExtent = {width, height, 1};  // Size of region to copy as(x, y, z) values

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(commandBuffer);
}

static void TransitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels = 1) {
	// Create buffer
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier imageMemoryBarrier            = {};
	imageMemoryBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout                       = oldLayout;                // Layout to transition from
	imageMemoryBarrier.newLayout                       = newLayout;                // Layout to transition to
	imageMemoryBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;  // Queue family to transition from
	imageMemoryBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;  // Queue family to transition to
	imageMemoryBarrier.image                           = image;  // Image being accessed and modified as part of barrier
	imageMemoryBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;  // Aspect of image being altered
	imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;                          // First mip level to start alterations on
	imageMemoryBarrier.subresourceRange.levelCount     = mipLevels;  // Number of mip levels to alter starting from baseMipLevel
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;  // First layer to start alterations on
	imageMemoryBarrier.subresourceRange.layerCount     = 1;  // Number of layers to alter stating from baseArrayLayer

	VkPipelineStageFlags srcStage{};
	VkPipelineStageFlags dstStage{};

	// If transitioning from new image to image ready to receive data...
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = 0;                             // Memory access stage transition must after...
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // Memory access stage transition must before...

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// If fransitioning from transfer destination to shader readable...
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		ENGINE_ERROR("Unsupported Layout Transition!");
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

	EndSingleTimeCommands(commandBuffer);
}

static void CreateImage(
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags useFlags,
	VkMemoryPropertyFlags propFlags,
	VkDeviceMemory& imageMemory,
	VkImage& image,
	u32 mipLevels = 1
) {
	// CREATE IMAGE
	// Image Creation Info
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;  // Type of image (1D, 2D, or 3D)
	imageCreateInfo.extent.width      = width;             // Width of image extent
	imageCreateInfo.extent.height     = height;            // Height of image extent
	imageCreateInfo.extent.depth      = 1;                 // Depth of image (just 1, no 3D aspect)
	imageCreateInfo.mipLevels         = mipLevels;          // Number of mipmap levels
	imageCreateInfo.arrayLayers       = 1;                 // Number of levels in image array
	imageCreateInfo.format            = format;            // Format type of image
	imageCreateInfo.tiling            = tiling;            // How many data should be "tiled" (arranged for optimal reading)
	imageCreateInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;  // Layout of image data on creation
	imageCreateInfo.usage             = useFlags;                   // Bit flags defining what image will be used for
	imageCreateInfo.samples           = VK_SAMPLE_COUNT_1_BIT;      // Number of samples for multi-sampling
	imageCreateInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;  // Whether image can be shared between queues

	// Create image
	VkResult result = vkCreateImage(VKContext->GetLogicalDevice(), &imageCreateInfo, nullptr, &image);
	VK_CHECK(result, "Failed to create an Image!");

	// CREATE MEMORY FOR IMAGE
	// Get memory requirements for a type of image
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(VKContext->GetLogicalDevice(), image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize       = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex      = FindMemoryType(memoryRequirements.memoryTypeBits, propFlags);

	result = vkAllocateMemory(VKContext->GetLogicalDevice(), &memoryAllocInfo, nullptr, &imageMemory);
	VK_CHECK(result, "Failed to allocate memory for image!");

	// Connect memory to image
	vkBindImageMemory(VKContext->GetLogicalDevice(), image, imageMemory, 0);
}

static void CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView, u32 mipLevels = 1) {
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image                 = image;                  // Image to create view for
	viewCreateInfo.viewType              = VK_IMAGE_VIEW_TYPE_2D;  // Type of image(1D, 2D, 3D, Cube, etc)
	viewCreateInfo.format                = format;                 // Format of image data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;   // Allows remapping of rgba components to other rgba values
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Subresources allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask =
		aspectFlags;                                     // Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
	viewCreateInfo.subresourceRange.baseMipLevel   = 0;  // Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount     = mipLevels;  // Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;  // Start array level to view from
	viewCreateInfo.subresourceRange.layerCount     = 1;  // Number of array levels to view

	// Create image view and return it
	// VkImageView imageView;
	VkResult result = vkCreateImageView(VKContext->GetLogicalDevice(), &viewCreateInfo, nullptr, &imageView);
	VK_CHECK(result, "Failed to create an Image View!");
}

static std::vector<char> ReadShaderFromAssets(const std::string& filename) {
	// Open stream from given file
	// std::ios::binary tells stream to read file as binary
	// std::ios::ate tells stream to start reading from the and of file
	std::string assetsPath = ASSETS_DIR + filename;

	std::ifstream file(assetsPath, std::ios::binary | std::ios::ate);

	// Check if file stream successfully opened
	if (!file.is_open()) {
		ENGINE_ERROR("Failed to open a file: " + assetsPath);
	}

	// Get current read position and use to resize file buffer
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> fileBuffer(fileSize);

	// Move read position (seek to) the start of the file
	file.seekg(0);

	// Read the file data into the buffer(stream "fileSize" in total)
	file.read(fileBuffer.data(), fileSize);

	// Close stream
	file.close();

	return fileBuffer;
}
}  // namespace Vulkan

//////////////////////////////////////////////////////////////////////////
// Vulkan validation
//////////////////////////////////////////////////////////////////////////
#ifdef RAVA_DEBUG
static const bool ENABLE_VALIDATION = true;
#else
static const bool ENABLE_VALIDATION = false;
#endif

// List of validation layers to use
// VK_LAYER_LUNARG_standard_validation = All standard validation layers
static const std::vector<const char*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

namespace Vulkan {
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			// If validation ERROR, then output error and return failure
			ENGINE_ERROR(pCallbackData->pMessage);
			return VK_TRUE;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			// If validation WARNING, then output warning and return okay
			ENGINE_WARN(pCallbackData->pMessage);
			return VK_FALSE;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			ENGINE_INFO(pCallbackData->pMessage);
			return VK_FALSE;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			ENGINE_TRACE(pCallbackData->pMessage);
			return VK_FALSE;
		default:
			return VK_FALSE;
	}
}

static VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void DestroyDebugUtilsMessengerEXT(
	VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator
) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo                 = {};
	createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
							   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
						   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}
}  // namespace Vulkan