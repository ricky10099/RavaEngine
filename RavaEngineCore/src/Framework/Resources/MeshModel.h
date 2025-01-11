#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/Buffer.h"

namespace Rava {
//class AssimpLoader;
class ufbxLoader;

struct Vertex {
	glm::vec3 position{};
	glm::vec4 color{};
	glm::vec3 normal{};
	glm::vec2 uv{};
	glm::vec3 tangent;
	glm::ivec4 jointIds;
	glm::vec4 weights;

	static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

	bool operator==(const Vertex& other) const {
		return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
	}
};

struct Mesh {
	u32 firstIndex;
	u32 firstVertex;
	u32 indexCount;
	u32 vertexCount;
	u32 instanceCount;
};

class MeshModel {
   public:
	//MeshModel(const AssimpLoader& loader);
	MeshModel(const ufbxLoader& loader);
	~MeshModel() = default;

	NO_COPY(MeshModel)

	static Unique<MeshModel> CreateMeshModelFromFile(const std::string_view filepath);

	void Bind(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout);
	void Draw(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout);
	void DrawMesh(const VkCommandBuffer& commandBuffer, const Mesh& mesh) const;

	//std::shared_ptr<Skeleton> GetSkeleton() const { return m_skeleton; }
	//std::shared_ptr<RVKBuffer> GetSkeletonBuffer() { return m_skeletonBuffer; }

   private:
	std::vector<Mesh> m_meshes{};

	Unique<Vulkan::Buffer> m_vertexBuffer;
	u32 m_vertexCount;

	bool m_hasIndexBuffer = false;
	Unique<Vulkan::Buffer> m_indexBuffer;
	u32 m_indexCount;

   private:
	// Skeleton
	//Shared<Skeleton> m_skeleton;
	//Shared<RVKBuffer> m_skeletonBuffer;

   private:
	void CopyMeshes(const std::vector<Mesh>& meshes);

	void CreateVertexBuffers(const std::vector<Vertex>& vertices);
	void CreateIndexBuffers(const std::vector<u32>& indices);

	void BindDescriptors(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, Mesh& mesh);
	//void PushConstantsPbr(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, const Mesh& mesh);
};
}  // namespace RVK