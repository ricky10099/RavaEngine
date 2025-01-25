#pragma once

#include "Framework/PhysicsUtils.h"

namespace Rava {
class MeshModel;
class Scene;
class PhysicsSystem {
   public:
	enum class ColliderType {
		Box,
		Sphere,
		Capsule,
		Plane,
		ConvexMesh,
		TriangleMesh,
	};

	enum class RigidBodyType {
		Static,
		Dynamic,
	};

   public:
	PhysicsSystem();
	~PhysicsSystem();

	void DisconnectPVD();

	physx::PxScene* CreatePhysXScene();
	physx::PxTriangleMesh* CreateTriangleMesh(const MeshModel& mesh);
	physx::PxConvexMesh* CreateConvexMesh(const MeshModel& mesh);

	//static std::vector<Vertex> CreateWireframeVertices(physx::PxShape* shape) {
	//	std::vector<Vertex> wireframeVertices;
	//	const physx::PxGeometry& geometry = shape->getGeometry();

	//	// Get the world transform of the shape
	//	physx::PxTransform shapeTransform = shape->getLocalPose();

	//	// Color for wireframes (you can modify this based on collision state)
	//	glm::vec4 wireframeColor(1.0f, 0.0f, 0.0f, 1.0f);  // Red wireframe

	//	// Handle different geometry types
	//	switch (geometry.getType()) {
	//		case physx::PxGeometryType::eBOX: {
	//			// Cast to box geometry
	//			const physx::PxBoxGeometry& boxGeom = (const physx::PxBoxGeometry&)geometry;
	//			physx::PxVec3 halfExtents     = boxGeom.halfExtents;

	//			// Generate 8 box corner points
	//			std::vector<physx::PxVec3> corners = {
	//				physx::PxVec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
	//				physx::PxVec3(halfExtents.x, -halfExtents.y, -halfExtents.z),
	//				physx::PxVec3(halfExtents.x, halfExtents.y, -halfExtents.z),
	//				physx::PxVec3(-halfExtents.x, halfExtents.y, -halfExtents.z),
	//				physx::PxVec3(-halfExtents.x, -halfExtents.y, halfExtents.z),
	//				physx::PxVec3(halfExtents.x, -halfExtents.y, halfExtents.z),
	//				physx::PxVec3(halfExtents.x, halfExtents.y, halfExtents.z),
	//				physx::PxVec3(-halfExtents.x, halfExtents.y, halfExtents.z)
	//			};

	//			// Box wireframe line indices
	//			std::vector<std::pair<int, int>> edges = {
	//				{0, 1},
	//				{1, 2},
	//				{2, 3},
	//				{3, 0}, // Bottom face
	//				{4, 5},
	//				{5, 6},
	//				{6, 7},
	//				{7, 4}, // Top face
	//				{0, 4},
	//				{1, 5},
	//				{2, 6},
	//				{3, 7}  // Connecting edges
	//			};

	//			// Transform and add vertices
	//			for (auto& edgePair : edges) {
	//				physx::PxVec3 start = shapeTransform.transform(corners[edgePair.first]);
	//				physx::PxVec3 end   = shapeTransform.transform(corners[edgePair.second]);

	//				wireframeVertices.push_back({glm::vec3(start.x, start.y, start.z), wireframeColor});
	//				wireframeVertices.push_back({glm::vec3(end.x, end.y, end.z), wireframeColor});
	//			}
	//			break;
	//		}

	//		//case PxGeometryType::eSPHERE: {
	//		//	// Similar approach for sphere, but create circle-like wireframes
	//		//	PxSphereGeometry* sphereGeom = static_cast<PxSphereGeometry*>(&geometry);
	//		//	float radius                 = sphereGeom->radius;

	//		//	// Implement sphere wireframe generation
	//		//	// This would involve creating circles in different planes
	//		//	break;
	//		//}

	//		//case PxGeometryType::eCAPSULE: {
	//		//	// Handle capsule wireframe generation
	//		//	PxCapsuleGeometry* capsuleGeom = static_cast<PxCapsuleGeometry*>(&geometry);
	//		//	// Generate line segments representing capsule's central axis and circular end caps
	//		//	break;
	//		//}

	//			// Add more geometry types as needed
	//	}

	//	return wireframeVertices;
	//}

	void Update(Scene* scene, float deltaTime) const;

	void Pause() { m_paused = true; }
	void Unpause() { m_paused = false; }

	physx::PxPhysics& GetPhysics() { return *m_physics; }
	physx::PxDefaultCpuDispatcher* GetDispatcher() { return m_dispatcher; }
	physx::PxMaterial* GetDefaultPxMaterial() { return m_defaultMaterial; }
	bool IsPaused() const { return m_paused; }

   private:
	static constexpr physx::PxU32 THREADS = 4;
	physx::PxDefaultAllocator m_defaultAllocator;
	physx::PxDefaultErrorCallback m_defaultErrorCallback;
	physx::PxFoundation* m_foundation           = nullptr;
	physx::PxPvdTransport* m_pvdTransport       = nullptr;
	physx::PxPvd* m_pvd                         = nullptr;
	physx::PxPhysics* m_physics                 = nullptr;
	physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
	physx::PxScene* m_scene                     = nullptr;
	physx::PxMaterial* m_defaultMaterial        = nullptr;
	bool m_paused                               = false;

	// private:
	// void Simulate(physx::PxScene& pxScene);
};
}  // namespace Rava