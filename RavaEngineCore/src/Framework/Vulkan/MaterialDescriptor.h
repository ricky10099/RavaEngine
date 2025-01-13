#pragma once

#include "Framework/Resources/Materials.h"

namespace Vulkan {
class MaterialDescriptor {
   public:
	MaterialDescriptor(Rava::Material& material, Rava::Material::MaterialTextures& textures);
	MaterialDescriptor(MaterialDescriptor const& other);
	virtual ~MaterialDescriptor() = default;

   public:
	const VkDescriptorSet& GetDescriptorSet() const { return m_descriptorSet; }

   private:
	VkDescriptorSet m_descriptorSet;
};
}  // namespace Vulkan