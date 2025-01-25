#pragma once

namespace Rava {
class Texture {
   public:
	static constexpr bool USE_SRGB  = true;
	static constexpr bool USE_UNORM = false;

   public:
	Texture(bool nearestFilter = false);
	Texture(u32 id, int internalFormat, int dataFormat, int type);
	~Texture();

	bool Init(const u32 width, const u32 height, bool sRGB, const void* data, int minFilter, int magFilter);
	bool Init(const std::string& fileName, bool sRGB, bool flip = true);
	bool Init(const unsigned char* data, int length, bool sRGB);

	void SetFilename(const std::string& filename) { m_fileName = filename; }

	VkDescriptorImageInfo& GetDescriptorImageInfo() { return m_descriptorImageInfo; }
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	VkImage& GetImage() { return m_textureImage; }
	VkImageView& GetImageView() { return m_imageView; }
	VkSampler& GetSampler() { return m_sampler; }
	VkDescriptorSet& GetDescriptorSet() { return m_descriptorSet; }

   private:
	std::string m_fileName = "";
	u32 m_rendererID = 0;
	u8* m_localBuffer = 0;
	int m_width            = 0;
	int m_height           = 0;
	int m_bytesPerPixel    = 0;
	u32 m_mipLevels = 0;

	int m_internalFormat = 0;
	int m_dataFormat     = 0;
	bool m_sRGB = false;
	VkFilter m_minFilter = VK_FILTER_NEAREST;
	VkFilter m_magFilter = VK_FILTER_NEAREST;
	VkFilter m_minFilterMip = VK_FILTER_LINEAR;
	int m_type = 0;

	VkFormat m_imageFormat{};
	VkImage m_textureImage{};
	VkDeviceMemory m_textureImageMemory{};
	VkImageLayout m_imageLayout{};
	VkImageView m_imageView{};
	VkSampler m_sampler{};

	VkDescriptorImageInfo m_descriptorImageInfo{};
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

   private:
	static constexpr int TEXTURE_FILTER_NEAREST                = 9728;
	static constexpr int TEXTURE_FILTER_LINEAR                 = 9729;
	static constexpr int TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST = 9984;
	static constexpr int TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST  = 9985;
	static constexpr int TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR  = 9986;
	static constexpr int TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR   = 9987;

   private:
	bool Create();
	void GenerateMipmaps();

	VkFilter SetFilter(int minMagFilter);
	VkFilter SetFilterMip(int minFilter);
};
}  // namespace Rava