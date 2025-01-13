#include "ravapch.h"

#include "Framework/RavaEngine.h"
#include "Framework/Vulkan/MaterialDescriptor.h"
#include "Framework/Vulkan/Descriptor.h"
#include "Framework/Resources/Texture.h"
// #include "Framework/Resources/Materials.h"

extern std::shared_ptr<Rava::Texture> g_DefaultTexture;

namespace Vulkan {
MaterialDescriptor::MaterialDescriptor(Rava::Material& material, Rava::Material::MaterialTextures& textures) {
	material.materialBuffer = std::make_shared<Buffer>(
		sizeof(Rava::Material::PBRMaterial),
		1,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		VKContext->properties.limits.minUniformBufferOffsetAlignment
	);
	material.materialBuffer->Map();

	// textures
	std::shared_ptr<Rava::Texture> diffuseMap;
	std::shared_ptr<Rava::Texture> normalMap;
	std::shared_ptr<Rava::Texture> roughnessMetallicMap;
	std::shared_ptr<Rava::Texture> emissiveMap;
	std::shared_ptr<Rava::Texture> roughnessMap;
	std::shared_ptr<Rava::Texture> metallicMap;
	std::shared_ptr<Rava::Texture>& dummy = g_DefaultTexture;
	// std::shared_ptr<Rava::Texture> dummy = nullptr;
	// bool success                   = dummy->Init("../models/checker.png", Texture::USE_SRGB);

	diffuseMap = textures[Rava::Material::DIFFUSE_MAP_INDEX] ? textures[Rava::Material::DIFFUSE_MAP_INDEX] : dummy;
	normalMap  = textures[Rava::Material::NORMAL_MAP_INDEX] ? textures[Rava::Material::NORMAL_MAP_INDEX] : dummy;
	roughnessMetallicMap =
		textures[Rava::Material::ROUGHNESS_METALLIC_MAP_INDEX] ? textures[Rava::Material::ROUGHNESS_METALLIC_MAP_INDEX] : dummy;
	emissiveMap  = textures[Rava::Material::EMISSIVE_MAP_INDEX] ? textures[Rava::Material::EMISSIVE_MAP_INDEX] : dummy;
	roughnessMap = textures[Rava::Material::ROUGHNESS_MAP_INDEX] ? textures[Rava::Material::ROUGHNESS_MAP_INDEX] : dummy;
	metallicMap  = textures[Rava::Material::METALLIC_MAP_INDEX] ? textures[Rava::Material::METALLIC_MAP_INDEX] : dummy;

	{
		DescriptorSetLayout::Builder builder{};
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.AddBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		std::unique_ptr<DescriptorSetLayout> localDescriptorSetLayout = builder.Build();

		auto bufferInfo  = material.materialBuffer->DescriptorInfo();
		auto& imageInfo0 = static_cast<Rava::Texture*>(diffuseMap.get())->GetDescriptorImageInfo();
		auto& imageInfo1 = static_cast<Rava::Texture*>(normalMap.get())->GetDescriptorImageInfo();
		auto& imageInfo2 = static_cast<Rava::Texture*>(roughnessMetallicMap.get())->GetDescriptorImageInfo();
		auto& imageInfo3 = static_cast<Rava::Texture*>(emissiveMap.get())->GetDescriptorImageInfo();
		auto& imageInfo4 = static_cast<Rava::Texture*>(roughnessMap.get())->GetDescriptorImageInfo();
		auto& imageInfo5 = static_cast<Rava::Texture*>(metallicMap.get())->GetDescriptorImageInfo();

		DescriptorWriter descriptorWriter(*localDescriptorSetLayout, *Renderer::s_descriptorPool);
		descriptorWriter.WriteBuffer(0, &bufferInfo)
			.WriteImage(1, &imageInfo0)
			.WriteImage(2, &imageInfo1)
			.WriteImage(3, &imageInfo2)
			.WriteImage(4, &imageInfo3)
			.WriteImage(5, &imageInfo4)
			.WriteImage(6, &imageInfo5);
		descriptorWriter.Build(m_descriptorSet);
	}
}

MaterialDescriptor::MaterialDescriptor(MaterialDescriptor const& other) {
	m_descriptorSet = other.m_descriptorSet;
}
}  // namespace Vulkan