#include "ravapch.h"

#include "Framework/PhysicsSystem.h"
#include "Framework/RavaEngine.h"
#include "Framework/Components.h"

namespace Rava {
PhysicsSystem::PhysicsSystem()
	: m_foundation(PxCreateFoundation(PX_PHYSICS_VERSION, m_defaultAllocator, m_defaultErrorCallback))
	, m_pvdTransport(physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10))
	, m_pvd(PxCreatePvd(*m_foundation))
	, m_physics(PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true, m_pvd))
	, m_dispatcher(physx::PxDefaultCpuDispatcherCreate(THREADS))
	, m_defaultMaterial(m_physics->createMaterial(0.8f, 0.8f, 0.6f)) {
	m_pvd->disconnect();

	ENGINE_INFO("Physics System initialized");
}

PhysicsSystem::~PhysicsSystem() {
	m_defaultMaterial->release();
	m_dispatcher->release();
	m_physics->release();
	if (m_pvd) {
		m_pvd->disconnect();
		m_pvd->release();
		m_pvdTransport->release();
	}
	m_foundation->release();
}

void PhysicsSystem::DisconnectPVD() {
	m_pvd->disconnect();
}

physx::PxScene* PhysicsSystem::CreatePhysXScene() {
	physx::PxSceneDesc desc(m_physics->getTolerancesScale());
	desc.gravity       = {0.0f, -9.81f, 0.0f};
	desc.cpuDispatcher = m_dispatcher;
	desc.filterShader  = physx::PxDefaultSimulationFilterShader;

	const auto scene = m_physics->createScene(desc);

#ifndef NDEBUG
	if (const auto pvdClient = scene->getScenePvdClient()) {
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	if (m_pvd->connect(*m_pvdTransport, physx::PxPvdInstrumentationFlag::eALL)) {
		ENGINE_INFO("Connected to PhysX Visual Debugger");
	} else {
		ENGINE_INFO("Unable to connect to PhysX Visual Debugger");
	}
#endif

	// m_simCallback = std::make_unique<SimCallback>();
	// scene->setSimulationEventCallback(&*m_simCallback);

	return scene;
}
physx::PxTriangleMesh* PhysicsSystem::CreateTriangleMesh(const MeshModel& mesh) {
	return nullptr;
}
physx::PxConvexMesh* PhysicsSystem::CreateConvexMesh(const MeshModel& mesh) {
	return nullptr;
}

void PhysicsSystem::Update(Scene* scene, float deltaTime) const {
	if (m_paused) {
		return;
	}

	auto actors = GetActors(*scene->GetPxScene());

	for (auto [entity, rigidBody] : scene->GetRegistry().view<Component::RigidBody>().each()) {
		const auto it = actors.find(rigidBody.actor.get());
		if (it == actors.end()) {
			scene->GetPxScene()->addActor(*rigidBody.actor);
		} else {
			actors.erase(it);
		}
	}

	// Remove obsolete physics actors from physics scene.
	for (const auto actor : actors) {
		scene->GetPxScene()->removeActor(*actor);
	}

	scene->GetPxScene()->simulate(deltaTime);
	scene->GetPxScene()->fetchResults(true);

	for (auto [entity, rigidBody, transform] : scene->GetRegistry().view<Component::RigidBody, Component::Transform>().each()) {
		//if (auto rigidDynamic = dynamic_cast<physx::PxRigidDynamic*>(rigidBody.actor)) {
		transform.position = ToVec3(rigidBody.actor->getGlobalPose().p) + rigidBody.offset.position;
			transform.rotation =
			glm::degrees(glm::eulerAngles(ToQuat(rigidBody.actor->getGlobalPose().q))) + rigidBody.offset.rotation;
			//entity.setTransform(entity.parentTransform().inverse() * transform);
		//}
	}
}
}  // namespace Rava