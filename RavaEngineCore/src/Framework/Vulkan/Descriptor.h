#pragma once

namespace Vulkan {
class DescriptorSetLayout {
   public:
	class Builder {
	   public:
		Builder() = default;

		Builder& AddBinding(u32 binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, u32 count = 1);

		size_t Size() const { return m_bindings.size(); }
		std::unique_ptr<DescriptorSetLayout> Build() const;

	   private:
		std::unordered_map<u32, VkDescriptorSetLayoutBinding> m_bindings{};
	};

	DescriptorSetLayout(std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings);
	~DescriptorSetLayout();

	NO_COPY(DescriptorSetLayout)

	VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

   private:
	VkDescriptorSetLayout m_descriptorSetLayout;
	std::unordered_map<u32, VkDescriptorSetLayoutBinding> m_bindings;

	friend class DescriptorWriter;
};

class DescriptorPool {
   public:
	class Builder {
	   public:
		Builder() = default;

		Builder& AddPoolSize(VkDescriptorType descriptorType, u32 count);
		Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
		Builder& SetMaxSets(u32 count);
		std::unique_ptr<DescriptorPool> Build() const;

	   private:
		std::vector<VkDescriptorPoolSize> m_poolSizes{};
		u32 m_maxSets                           = 1000;
		VkDescriptorPoolCreateFlags m_poolFlags = 0;
	};

	DescriptorPool(u32 maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes);
	~DescriptorPool();

	NO_COPY(DescriptorPool)

	bool AllocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

	void FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

	VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }

	void ResetPool();

   private:
	VkDescriptorPool m_descriptorPool;

	friend class DescriptorWriter;
};

class DescriptorWriter {
   public:
	DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

	DescriptorWriter& WriteBuffer(u32 binding, VkDescriptorBufferInfo* bufferInfo);
	DescriptorWriter& WriteImage(u32 binding, VkDescriptorImageInfo* imageInfo);

	bool Build(VkDescriptorSet& set);
	void Overwrite(VkDescriptorSet& set);

   private:
	DescriptorSetLayout& m_setLayout;
	DescriptorPool& m_pool;
	std::vector<VkWriteDescriptorSet> m_writes;
};
}  // namespace Vulkan