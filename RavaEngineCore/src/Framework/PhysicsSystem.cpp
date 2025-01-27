#include "ravapch.h"

#include "Framework/PhysicsSystem.h"
#include "Framework/RavaEngine.h"
#include "Framework/Components.h"

namespace Rava {
PhysicsSystem::PhysicsSystem()
	: m_foundation(PxCreateFoundation(PX_PHYSICS_VERSION, m_defaultAllocator, m_errorCallback))
	, m_pvdTransport(physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10))
	, m_pvd(PxCreatePvd(*m_foundation))
	, m_physics(PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true, m_pvd))
	, m_dispatcher(physx::PxDefaultCpuDispatcherCreate(THREADS))
	, m_defaultMaterial(m_physics->createMaterial(0.8f, 0.8f, 0.6f)) {
	m_pvd->disconnect();

	ENGINE_INFO("Physics System initialized");
}

PhysicsSystem::~PhysicsSystem() {
	if (m_physicsCallBack) {
		delete m_physicsCallBack;
	}

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

namespace {
physx::PxFilterFlags CustomFilterShader(
	physx::PxFilterObjectAttributes attributes0,
	physx::PxFilterData filterData0,
	physx::PxFilterObjectAttributes attributes1,
	physx::PxFilterData filterData1,
	physx::PxPairFlags& pairFlags,
	const void* constantBlock,
	physx::PxU32 constantBlockSize
) {
	// Enable contact generation
	pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

	// Add these flags to enable contact events
	pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
	pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
	pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
	pairFlags |= physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;

	return physx::PxFilterFlag::eDEFAULT;
}
}  // namespace

physx::PxScene* PhysicsSystem::CreatePhysXScene() {
	physx::PxSceneDesc desc(m_physics->getTolerancesScale());
	desc.gravity       = {0.0f, -9.81f, 0.0f};
	desc.cpuDispatcher = m_dispatcher;
	desc.filterShader  = CustomFilterShader;
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

	if (m_physicsCallBack) {
		delete m_physicsCallBack;
	}
	m_physicsCallBack = new PhysicsEventCallback;
	scene->setSimulationEventCallback(m_physicsCallBack);

	return scene;
}
physx::PxTriangleMesh* PhysicsSystem::CreateTriangleMesh(MeshModel& mesh) {
	std::vector<Vertex> vertices = mesh.GetVertices();
	std::vector<u32> indices     = mesh.GetIndices();

	physx::PxTriangleMeshDesc meshDesc = {};
	meshDesc.points.count              = (physx::PxU32)vertices.size();
	meshDesc.points.data               = vertices.data();
	meshDesc.points.stride             = sizeof(vertices[0]);
	meshDesc.triangles.count           = (physx::PxU32)(indices.size() / 3);
	meshDesc.triangles.data            = indices.data();
	meshDesc.triangles.stride          = 3 * sizeof(physx::PxU32);

	physx::PxTolerancesScale scale;
	physx::PxCookingParams cookParams(scale);

	physx::PxDefaultMemoryOutputStream writeBuffer;
	physx::PxTriangleMeshCookingResult::Enum result;
	bool status = PxCookTriangleMesh(cookParams, meshDesc, writeBuffer, &result);
	if (!status) {
		return NULL;
	}

	physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	return m_physics->createTriangleMesh(readBuffer);
}

physx::PxConvexMesh* PhysicsSystem::CreateConvexMesh(MeshModel& mesh) {
	std::vector<Vertex> vertices = mesh.GetVertices();

	physx::PxConvexMeshDesc convexDesc = {};
	convexDesc.points.count            = (physx::PxU32)vertices.size();
	convexDesc.points.data             = vertices.data();
	convexDesc.points.stride           = sizeof(vertices[0]);
	convexDesc.flags                   = physx::PxConvexFlag::eCOMPUTE_CONVEX;

	physx::PxTolerancesScale scale;
	physx::PxCookingParams params(scale);

	physx::PxDefaultMemoryOutputStream buf;
	physx::PxConvexMeshCookingResult::Enum result;
	if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
		return NULL;
	}

	physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
	return m_physics->createConvexMesh(input);
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
		transform.position = ToVec3(rigidBody.actor->getGlobalPose().p) + rigidBody.offset.position;
		transform.rotation = glm::eulerAngles(ToQuat(rigidBody.actor->getGlobalPose().q)) + rigidBody.offset.rotation;
	}
}
}  // namespace Rava