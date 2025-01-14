#include "ravapch.h"

#include "Framework/Resources/MeshModel.h"
#include "Framework/Resources/ufbxLoader.h"
#include "Framework/Vulkan/MaterialDescriptor.h"

namespace Rava {
std::vector<VkVertexInputBindingDescription> Vertex::GetBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding   = 0;
	bindingDescriptions[0].stride    = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)});
	attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
	attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});
	attributeDescriptions.push_back({4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)});
	attributeDescriptions.push_back({5, 0, VK_FORMAT_R32G32B32A32_SINT, offsetof(Vertex, jointIds)});
	attributeDescriptions.push_back({6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weights)});

	return attributeDescriptions;
}

Unique<MeshModel> MeshModel::CreateMeshModelFromFile(std::string_view filePath) {
	ufbxLoader loader{filePath.data()};
	if (!loader.Load()) {
		ENGINE_ERROR("Failed to load Model file {0}", filePath.data());
		return nullptr;
	}
	return std::make_unique<MeshModel>(loader);
}

MeshModel::MeshModel(const ufbxLoader& loader) {
	CopyMeshes(loader.meshes);
	CreateVertexBuffers(loader.vertices);
	CreateIndexBuffers(loader.indices);
	// m_skeleton       = std::move(builder.skeleton);
	// m_skeletonBuffer = builder.shaderData;
}

void MeshModel::CopyMeshes(const std::vector<Mesh>& meshes) {
	for (auto& mesh : meshes) {
		m_meshes.push_back(mesh);
	}
}

void MeshModel::CreateVertexBuffers(const std::vector<Vertex>& vertices) {
	m_vertexCount = static_cast<u32>(vertices.size());
	ENGINE_ASSERT(m_vertexCount >= 3, "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;
	u32 vertexSize          = sizeof(vertices[0]);

	Vulkan::Buffer stagingBuffer{
		vertexSize,
		m_vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer((void*)vertices.data());

	m_vertexBuffer = std::make_unique<Vulkan::Buffer>(
		vertexSize,
		m_vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	Vulkan::CopyBuffer(stagingBuffer.GetBuffer(), m_vertexBuffer->GetBuffer(), bufferSize);
}

void MeshModel::CreateIndexBuffers(const std::vector<u32>& indices) {
	m_indexCount     = static_cast<u32>(indices.size());
	m_hasIndexBuffer = m_indexCount > 0;

	if (!m_hasIndexBuffer) {
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;
	u32 indexSize           = sizeof(indices[0]);

	Vulkan::Buffer stagingBuffer{
		indexSize,
		m_indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer((void*)indices.data());

	m_indexBuffer = std::make_unique<Vulkan::Buffer>(
		indexSize,
		m_indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	Vulkan::CopyBuffer(stagingBuffer.GetBuffer(), m_indexBuffer->GetBuffer(), bufferSize);
}

void MeshModel::Bind(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout) {
	VkBuffer buffers[]     = {m_vertexBuffer->GetBuffer()};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(frameInfo.commandBuffer, 0, 1, buffers, offsets);

	if (m_hasIndexBuffer) {
		vkCmdBindIndexBuffer(frameInfo.commandBuffer, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void MeshModel::Draw(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout) {
	for (auto& mesh : m_meshes) {
		BindDescriptors(frameInfo, pipelineLayout, mesh);
		DrawMesh(frameInfo.commandBuffer, mesh);
	}
}

void MeshModel::DrawMesh(const VkCommandBuffer& commandBuffer, const Mesh& mesh) const {
	if (m_hasIndexBuffer) {
		vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, mesh.firstVertex, 0);
	} else {
		vkCmdDraw(commandBuffer, mesh.vertexCount, 1, mesh.firstVertex, 0);
	}
}

void MeshModel::BindDescriptors(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, Mesh& mesh) {
	mesh.material.materialBuffer->WriteToBuffer(&mesh.material.pbrMaterial);
	mesh.material.materialBuffer->Flush();

	const VkDescriptorSet& materialDescriptorSet = mesh.material.materialDescriptor->GetDescriptorSet();
	//const VkDescriptorSet& skeletonDescriptorSet = mesh.skeletonDescriptorSet;

	std::vector<VkDescriptorSet> descriptorSets = {frameInfo.globalDescriptorSet, materialDescriptorSet/*, skeletonDescriptorSet*/};
	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,                  // VkCommandBuffer        commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,          // VkPipelineBindPoint    pipelineBindPoint,
		pipelineLayout,                           // VkPipelineLayout       layout,
		0,                                        // uint32_t               firstSet,
		static_cast<u32>(descriptorSets.size()),  // uint32_t               descriptorSetCount,
		descriptorSets.data(),                    // const VkDescriptorSet* pDescriptorSets,
		0,                                        // uint32_t               dynamicOffsetCount,
		nullptr                                   // const uint32_t*        pDynamicOffsets);
	);
}

}  // namespace Rava