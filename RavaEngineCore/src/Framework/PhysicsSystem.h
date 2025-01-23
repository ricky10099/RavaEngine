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