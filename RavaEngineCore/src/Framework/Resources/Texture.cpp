#include "ravapch.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Resources/Texture.h"

namespace Rava {
Texture::Texture(bool nearestFilter) {
	nearestFilter ? m_minFilter = VK_FILTER_NEAREST : m_minFilter = VK_FILTER_LINEAR;
	nearestFilter ? m_magFilter = VK_FILTER_NEAREST : m_magFilter = VK_FILTER_LINEAR;
	m_minFilterMip = VK_FILTER_LINEAR;
}
Texture::Texture(u32 ID, int internalFormat, int dataFormat, int type)
	: m_rendererID{ID}
	, m_internalFormat{internalFormat}
	, m_dataFormat{dataFormat}
	, m_type{type}
	, m_sRGB{false} {}

Texture::~Texture() {
	vkDestroyImage(VKContext->GetLogicalDevice(), m_textureImage, nullptr);
	vkDestroyImageView(VKContext->GetLogicalDevice(), m_imageView, nullptr);
	vkDestroySampler(VKContext->GetLogicalDevice(), m_sampler, nullptr);
	vkFreeMemory(VKContext->GetLogicalDevice(), m_textureImageMemory, nullptr);
}

// create texture from raw memory
bool Texture::Init(const u32 width, const u32 height, bool sRGB, const void* data, int minFilter, int magFilter) {
	bool ok        = false;
	m_fileName     = "raw memory";
	m_sRGB         = sRGB;
	m_localBuffer  = (u8*)data;
	m_minFilter    = SetFilter(minFilter);
	m_magFilter    = SetFilter(magFilter);
	m_minFilterMip = SetFilterMip(minFilter);

	if (m_localBuffer) {
		m_width         = width;
		m_height        = height;
		m_bytesPerPixel = 4;
		ok              = Create();
	}
	return ok;
}

// create texture from file on disk
bool Texture::Init(const std::string& fileName, bool sRGB, bool flip) {
	bool ok = false;
	stbi_set_flip_vertically_on_load(flip);
	m_fileName    = fileName;
	m_sRGB        = sRGB;
	m_localBuffer = stbi_load(m_fileName.c_str(), &m_width, &m_height, &m_bytesPerPixel, 4);

	if (m_localBuffer) {
		ok = Create();
		stbi_image_free(m_localBuffer);
	} else {
		ENGINE_ERROR("Texture: Couldn't load file {0}", fileName);
	}

	return ok;
}

// create texture from file in memory
bool Texture::Init(const unsigned char* data, int length, bool sRGB) {
	bool ok = false;
	stbi_set_flip_vertically_on_load(true);
	m_fileName    = "file in memory";
	m_sRGB        = sRGB;
	m_localBuffer = stbi_load_from_memory(data, length, &m_width, &m_height, &m_bytesPerPixel, 4);

	if (m_localBuffer) {
		ok = Create();
		stbi_image_free(m_localBuffer);
	} else {
		std::cout << "Texture: Couldn't load file " << m_fileName << std::endl;
	}
	return ok;
}

bool Texture::Create() {
	VkDeviceSize imageSize = m_width * m_height * 4;

	if (!m_localBuffer) {
		ENGINE_ERROR("Failed to load Texture Image!");
		return false;
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Vulkan::CreateBuffer(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(VKContext->GetLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, m_localBuffer, static_cast<size_t>(imageSize));
	vkUnmapMemory(VKContext->GetLogicalDevice(), stagingBufferMemory);

	m_imageFormat = m_sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	m_mipLevels   = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;
	Vulkan::CreateImage(
		m_width,
		m_height,
		m_imageFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_textureImageMemory,
		m_textureImage,
		m_mipLevels
	);

	Vulkan::TransitionImageLayout(m_textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);

	Vulkan::CopyBufferToImage(
		stagingBuffer, m_textureImage, static_cast<u32>(m_width), static_cast<u32>(m_height), 1 /*layerCount*/
	);

	GenerateMipmaps();

	m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vkDestroyBuffer(VKContext->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(VKContext->GetLogicalDevice(), stagingBufferMemory, nullptr);

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter        = m_magFilter;
	samplerCreateInfo.minFilter        = m_minFilter;
	samplerCreateInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.compareOp        = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.mipLodBias       = 0.0f;
	samplerCreateInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.minLod           = 0.0f;
	samplerCreateInfo.maxLod           = static_cast<float>(m_mipLevels);
	samplerCreateInfo.maxAnisotropy    = 4.0;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	{
		auto result = vkCreateSampler(VKContext->GetLogicalDevice(), &samplerCreateInfo, nullptr, &m_sampler);
		if (result != VK_SUCCESS) {
			ENGINE_ERROR("Failed to create Sampler!");
		}
	}

	Vulkan::CreateImageView(m_textureImage, m_imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_imageView, m_mipLevels);

	m_descriptorImageInfo.sampler     = m_sampler;
	m_descriptorImageInfo.imageView   = m_imageView;
	m_descriptorImageInfo.imageLayout = m_imageLayout;

	// Check image handles
	if (m_textureImage == VK_NULL_HANDLE) {
		ENGINE_ERROR("Invalid Vulkan Image Handle");
	}

	if (m_imageView == VK_NULL_HANDLE) {
		ENGINE_ERROR("Invalid Vulkan Image View Handle");
	}

	if (m_sampler == VK_NULL_HANDLE) {
		ENGINE_ERROR("Invalid Vulkan Sampler Handle");
	}

	return true;
}

void Texture::GenerateMipmaps() {
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(VKContext->GetPhysicalDevice(), m_imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		ENGINE_WARN("texture image format does not support linear blitting!");
		return;
	}

	VkCommandBuffer commandBuffer = Vulkan::BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image                           = m_textureImage;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;
	barrier.subresourceRange.levelCount     = 1;

	int mipWidth  = m_width;
	int mipHeight = m_height;

	for (u32 i = 1; i < m_mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier
		);

		VkImageBlit blit{};
		blit.srcOffsets[0]                 = {0, 0, 0};
		blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
		blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel       = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount     = 1;
		blit.dstOffsets[0]                 = {0, 0, 0};
		blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
		blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel       = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount     = 1;

		vkCmdBlitImage(
			commandBuffer,
			m_textureImage,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_textureImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blit,
			m_minFilterMip
		);

		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		if (mipWidth > 1) {
			mipWidth /= 2;
		}
		if (mipHeight > 1) {
			mipHeight /= 2;
		}
	}

	barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
	barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	);

	Vulkan::EndSingleTimeCommands(commandBuffer);
}

VkFilter Texture::SetFilter(int minMagFilter) {
	VkFilter filter = VK_FILTER_LINEAR;
	switch (minMagFilter) {
		case TEXTURE_FILTER_NEAREST:
		case TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
		case TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: {
			filter = VK_FILTER_NEAREST;
			break;
		}
	}
	return filter;
}

VkFilter Texture::SetFilterMip(int minFilter) {
	VkFilter filter = VK_FILTER_LINEAR;
	switch (minFilter) {
		case TEXTURE_FILTER_NEAREST:
		case TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: {
			break;
		}
		case TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: {
			break;
		}
			{
				filter = VK_FILTER_NEAREST;
				break;
			}
	}
	return filter;
}
}  // namespace Rava