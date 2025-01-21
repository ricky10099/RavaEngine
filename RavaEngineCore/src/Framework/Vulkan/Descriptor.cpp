#include "ravapch.h"

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/Descriptor.h"

namespace Vulkan {
// *************** Descriptor Set Layout Builder *********************
DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddBinding(
	u32 binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, u32 count
) {
	ENGINE_ASSERT(m_bindings.count(binding) == 0, "Binding already in use");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding         = binding;
	layoutBinding.descriptorType  = descriptorType;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags      = stageFlags;
	m_bindings[binding]           = layoutBinding;
	return *this;
}

std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::Build() const {
	return std::make_unique<DescriptorSetLayout>(m_bindings);
}

// *************** Descriptor Set Layout *********************
DescriptorSetLayout::DescriptorSetLayout(std::unordered_map<u32, VkDescriptorSetLayoutBinding> bindings)
	: m_bindings{bindings} {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
	for (auto& it : bindings) {
		setLayoutBindings.push_back(it.second);
	}

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<u32>(setLayoutBindings.size());
	descriptorSetLayoutInfo.pBindings    = setLayoutBindings.data();

	VkResult result =
		vkCreateDescriptorSetLayout(VKContext->GetLogicalDevice(), &descriptorSetLayoutInfo, nullptr, &m_descriptorSetLayout);
	VK_CHECK(result, "Failed to Create Descriptor Set Layout!");
}

DescriptorSetLayout::~DescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(VKContext->GetLogicalDevice(), m_descriptorSetLayout, nullptr);
}

// *************** Descriptor Pool Builder *********************
DescriptorPool::Builder& DescriptorPool::Builder::AddPoolSize(VkDescriptorType descriptorType, u32 count) {
	m_poolSizes.push_back({descriptorType, count});
	return *this;
}

DescriptorPool::Builder& DescriptorPool::Builder::SetPoolFlags(VkDescriptorPoolCreateFlags flags) {
	m_poolFlags = flags;
	return *this;
}
DescriptorPool::Builder& DescriptorPool::Builder::SetMaxSets(u32 count) {
	m_maxSets = count;
	return *this;
}

std::unique_ptr<DescriptorPool> DescriptorPool::Builder::Build() const {
	return std::make_unique<DescriptorPool>(m_maxSets, m_poolFlags, m_poolSizes);
}

// *************** Descriptor Pool *********************
DescriptorPool::DescriptorPool(
	u32 maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes
) {
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes    = poolSizes.data();
	descriptorPoolInfo.maxSets       = maxSets;
	descriptorPoolInfo.flags         = poolFlags;

	VkResult result = vkCreateDescriptorPool(VKContext->GetLogicalDevice(), &descriptorPoolInfo, nullptr, &m_descriptorPool);
	VK_CHECK(result, "Failed to Create Descriptor Pool!");
}

DescriptorPool::~DescriptorPool() {
	vkDestroyDescriptorPool(VKContext->GetLogicalDevice(), m_descriptorPool, nullptr);
}

bool DescriptorPool::AllocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool     = m_descriptorPool;
	allocInfo.pSetLayouts        = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;

	// Might want to create a "DescriptorPoolManager" class that handles this case, and builds
	// a new pool whenever an old pool fills up. But this is beyond our current scope
	VkResult result = vkAllocateDescriptorSets(VKContext->GetLogicalDevice(), &allocInfo, &descriptor);
	if (result != VK_SUCCESS) {
		return false;
	}
	return true;
}

void DescriptorPool::FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
	vkFreeDescriptorSets(
		VKContext->GetLogicalDevice(), m_descriptorPool, static_cast<u32>(descriptors.size()), descriptors.data()
	);
}

void DescriptorPool::ResetPool() {
	vkResetDescriptorPool(VKContext->GetLogicalDevice(), m_descriptorPool, 0);
}

// *************** Descriptor Writer *********************
DescriptorWriter::DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool)
	: m_setLayout{setLayout}
	, m_pool{pool} {}

DescriptorWriter& DescriptorWriter::WriteBuffer(u32 binding, VkDescriptorBufferInfo* bufferInfo) {
	ENGINE_ASSERT(m_setLayout.m_bindings.count(binding) == 1, "Layout does not contain specified binding!");

	auto& bindingDescription = m_setLayout.m_bindings[binding];

	ENGINE_ASSERT(bindingDescription.descriptorCount == 1, "Binding Single Descriptor Info, but Binding expects multiple!");

	VkWriteDescriptorSet write{};
	write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType  = bindingDescription.descriptorType;
	write.dstBinding      = binding;
	write.pBufferInfo     = bufferInfo;
	write.descriptorCount = 1;

	m_writes.push_back(write);
	return *this;
}

DescriptorWriter& DescriptorWriter::WriteImage(u32 binding, VkDescriptorImageInfo* imageInfo) {
	ENGINE_ASSERT(m_setLayout.m_bindings.count(binding) == 1, "Layout does not contain specified binding!");

	auto& bindingDescription = m_setLayout.m_bindings[binding];

	ENGINE_ASSERT(bindingDescription.descriptorCount == 1, "Binding Single Ddescriptor Info, but Binding expects multiple");

	VkWriteDescriptorSet write{};
	write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType  = bindingDescription.descriptorType;
	write.dstBinding      = binding;
	write.pImageInfo      = imageInfo;
	write.descriptorCount = 1;

	m_writes.push_back(write);
	return *this;
}

bool DescriptorWriter::Build(VkDescriptorSet& set) {
	bool success = m_pool.AllocateDescriptor(m_setLayout.GetDescriptorSetLayout(), set);
	if (!success) {
		return false;
	}
	Overwrite(set);
	return true;
}

void DescriptorWriter::Overwrite(VkDescriptorSet& set) {
	for (auto& write : m_writes) {
		write.dstSet = set;
	}
	vkUpdateDescriptorSets(VKContext->GetLogicalDevice(), static_cast<u32>(m_writes.size()), m_writes.data(), 0, nullptr);
}
}  // namespace Vulkan