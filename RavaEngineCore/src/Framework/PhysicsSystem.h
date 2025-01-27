#pragma once

#include "Framework/PhysicsUtils.h"

namespace Rava {
class MeshModel;
class Scene;

class PhysicsErrorCallback : public physx::PxErrorCallback {
   public:
	virtual void reportError(physx::PxErrorCode::Enum code, const char* msg, const char* file, int line) override;
};

class PhysicsEventCallback : public physx::PxSimulationEventCallback {
   public:
	virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
		override;
	virtual void onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, const physx::PxU32) override {}
	virtual void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) override {}
	virtual void onWake(physx::PxActor**, physx::PxU32) override {}
	virtual void onSleep(physx::PxActor**, physx::PxU32) override {}
};

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

   public:
	PhysicsSystem();
	~PhysicsSystem();

	void DisconnectPVD();

	physx::PxScene* CreatePhysXScene();
	physx::PxTriangleMesh* CreateTriangleMesh(MeshModel& mesh);
	physx::PxConvexMesh* CreateConvexMesh(MeshModel& mesh);

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
	PhysicsErrorCallback m_errorCallback;
	physx::PxFoundation* m_foundation           = nullptr;
	physx::PxPvdTransport* m_pvdTransport       = nullptr;
	physx::PxPvd* m_pvd                         = nullptr;
	physx::PxPhysics* m_physics                 = nullptr;
	physx::PxDefaultCpuDispatcher* m_dispatcher = nullptr;
	PhysicsEventCallback* m_physicsCallBack     = nullptr;
	physx::PxScene* m_scene                     = nullptr;
	physx::PxMaterial* m_defaultMaterial        = nullptr;
	bool m_paused                               = false;
};
}  // namespace Rava