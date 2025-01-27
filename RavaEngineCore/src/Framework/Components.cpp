#include "ravapch.h"

#include "Framework/Components.h"

namespace Rava::Component {
RigidBody::RigidBody(
	Entity& entity, PhysicsSystem::ColliderType colliderType, bool isTrigger, bool isDynamic, physx::PxMaterial* pxMaterial
) {
	if (!material) {
		material           = Engine::s_Instance->GetPhysicsSystem().GetDefaultPxMaterial();
		useDefaultMaterial = true;
	}

	physx::PxTransform localTm(
		ToTransform(entity.GetComponent<Transform>()->position, entity.GetComponent<Transform>()->GetQuaternion())
	);

	if (isDynamic) {
		actor           = WrapUnique(Engine::s_Instance->GetPhysicsSystem().GetPhysics().createRigidDynamic(localTm));
		actor->userData = reinterpret_cast<void*>(&entity);
	} else {
		actor           = WrapUnique(Engine::s_Instance->GetPhysicsSystem().GetPhysics().createRigidStatic(localTm));
		actor->userData = reinterpret_cast<void*>(&entity);
	}

	auto flags =
		physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE;
	if (isTrigger) {
		flags = physx::PxShapeFlag::eTRIGGER_SHAPE;
	}

	glm::vec3 lower{}, upper{};
	MeshModel* meshModel = nullptr;
	float width                = 1.0f;
	bool hasModel              = entity.HasComponent<Component::Model>();
	if (hasModel) {
		meshModel = entity.GetComponent<Model>()->model.get();
		lower     = meshModel->GetBounds().lower;
		upper     = meshModel->GetBounds().upper;
		width     = meshModel->GetWidth();
	}

	switch (colliderType) {
		case PhysicsSystem::ColliderType::Box: {
			physx::PxBoxGeometry box;
			if (hasModel) {
				box = physx::PxBoxGeometry(ToVec3(entity.GetScale() * (upper - lower) / 2.0f));
			} else {
				box = physx::PxBoxGeometry(0.5f, 0.5f, 0.5f);
			}
			physx::PxRigidActorExt::createExclusiveShape(*actor, box, *material, flags);
			break;
		}
		case PhysicsSystem::ColliderType::Sphere: {
			physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxSphereGeometry(width / 2.0f), *material, flags);
			break;
		}
		case PhysicsSystem::ColliderType::Capsule: {
			if (hasModel) {
				physx::PxRigidActorExt::createExclusiveShape(
					*actor, physx::PxCapsuleGeometry(width / 2.0f, upper.y - lower.y - width), *material, flags
				);
			} else {
				physx::PxRigidActorExt::createExclusiveShape(
					*actor, physx::PxCapsuleGeometry(width / 2.0f, 1.0f), *material, flags
				);
			}
			break;
		}
		case PhysicsSystem::ColliderType::Plane: {
			auto shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxPlaneGeometry(), *material, flags);
			shape->setLocalPose(physx::PxTransformFromPlaneEquation(physx::PxPlane(ToVec3(meshModel->GetVertices()[0].normal), 0.0f)));
			break;
		}
		case PhysicsSystem::ColliderType::ConvexMesh: {
			const auto convexMesh = Engine::s_Instance->GetPhysicsSystem().CreateConvexMesh(*meshModel);
			physx::PxConvexMeshGeometry convexGeometry(convexMesh, physx::PxMeshScale(ToVec3(entity.GetScale())));
			physx::PxRigidActorExt::createExclusiveShape(*actor, convexGeometry, *material, flags);
			break;
		}
		case PhysicsSystem::ColliderType::TriangleMesh: {
			const auto triangleMesh =
				Engine::s_Instance->GetPhysicsSystem().CreateTriangleMesh(*meshModel);
			physx::PxTriangleMeshGeometry meshGeometry(triangleMesh, physx::PxMeshScale(ToVec3(entity.GetScale())));
			physx::PxRigidActorExt::createExclusiveShape(*actor, meshGeometry, *material, flags);
			break;
		}
	}
}

RigidBody::~RigidBody() {
	if (!useDefaultMaterial) {
		material->release();
	}

	auto flag = physx::PxActorTypeFlag::eRIGID_DYNAMIC | physx::PxActorTypeFlag::eRIGID_STATIC;

	if (Engine::s_Instance->GetCurrentPxScene()->getNbActors(flag) > 0) {
		Engine::s_Instance->GetCurrentPxScene()->removeActor(*actor);
	}
}
}  // namespace Rava::Component