#pragma once

#include "Framework/Vulkan/VKUtils.h"
#include "Framework/Vulkan/Buffer.h"
#include "Framework/Resources/Materials.h"

namespace Rava {
// class AssimpLoader;
class ufbxLoader;
struct Skeleton;
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
	Material material;
	Shared<Vulkan::Buffer> skeletonBuffer;
	VkDescriptorSet skeletonDescriptorSet;
};



// struct Node {
//	glm::mat4 transform;
//	i32 parentNode;
//	std::vector<i32> children;
//	i32 boneID = -1;
//	glm::mat4 boneOffset;
// };

class MeshModel {
   public:
	struct Bounds {
		glm::vec3 lower;
		glm::vec3 upper;
	};

   public:
	// MeshModel(const AssimpLoader& loader);
	MeshModel(const ufbxLoader& loader);
	~MeshModel();

	NO_COPY(MeshModel)

	static Unique<MeshModel> CreateMeshModelFromFile(std::string_view filepath);

	void UpdateAnimation(u32 frameCounter);

	void Bind(VkCommandBuffer commandBuffer);
	void Draw(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout);
	void DrawMesh(const VkCommandBuffer& commandBuffer, const Mesh& mesh) const;

	Bounds GetBounds() const;
	float GetWidth() const;
	const std::vector<Vertex> GetVertices() { return m_vertices; }
	const std::vector<u32> GetIndices() { return m_indices; }
	bool HasSkeleton() const { return m_skeleton ? true : false; }
	Shared<Skeleton> GetSkeleton() { return m_skeleton; }
	// std::shared_ptr<Skeleton> GetSkeleton() const { return m_skeleton; }
	// std::shared_ptr<RVKBuffer> GetSkeletonBuffer() { return m_skeletonBuffer; }

   private:
	std::vector<Mesh> m_meshes{};
	// std::vector<Node> m_nodes{};
	std::map<std::string, i32> nodeMap;
	std::vector<Vertex> m_vertices;
	std::vector<u32> m_indices;

	Unique<Vulkan::Buffer> m_vertexBuffer;
	u32 m_vertexCount;

	bool m_hasIndexBuffer = false;
	Unique<Vulkan::Buffer> m_indexBuffer;
	u32 m_indexCount;

   private:
	void CopyMeshes(const std::vector<Mesh>& meshes);

	void CreateVertexBuffers(const std::vector<Vertex>& vertices);
	void CreateIndexBuffers(const std::vector<u32>& indices);

	void BindDescriptors(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, Mesh& mesh);
	// void PushConstantsPbr(const FrameInfo& frameInfo, const VkPipelineLayout& pipelineLayout, const Mesh& mesh);

   private:
	Shared<Skeleton> m_skeleton;
	Shared<Vulkan::Buffer> m_skeletonUbo;
};
}  // namespace Rava