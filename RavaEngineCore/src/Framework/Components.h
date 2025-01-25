#pragma once

#include "ravapch.h"
#include "Framework/RavaEngine.h"
#include "Framework/Resources/MeshModel.h"
#include "Framework/Resources/Animations.h"
#include "Framework/Camera.h"
#include "Framework/PhysicsSystem.h"
#include "Framework/Entity.h"

namespace Rava::Component {
struct Name {
	std::string data = "Empty Entity";

	Name()            = default;
	Name(const Name&) = default;
	Name(std::string_view name)
		: data(name) {}

	void SetName(std::string_view name) { data = name; }
};

struct Transform {
	glm::vec3 position = {0.0f, 0.0f, 0.0f};
	glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
	glm::vec3 scale    = {1.0f, 1.0f, 1.0f};

	Transform()                 = default;
	Transform(const Transform&) = default;
	Transform(const glm::vec3& pos)
		: position(pos) {}
	Transform(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& sca)
		: position(pos)
		, rotation(rot)
		, scale(sca) {}

	glm::mat4 GetTransform() const {
		return glm::translate(glm::mat4(1.0f), position) * glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f))
			 * glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f))
			 * glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), scale);
	}

	glm::mat3 NormalMatrix() const {
		return glm::mat3_cast(glm::quat(rotation)) * (glm::mat3)glm::scale(glm::mat4(1.0f), 1.0f / scale);
	}

	Transform* SetPosition(const glm::vec3& dstPosition) {
		position = dstPosition;
		return this;
	}
	Transform* SetRotation(const glm::vec3& dstRotation) {
		rotation = dstRotation;
		return this;
	}
	Transform* SetScale(const glm::vec3& dstScale) {
		scale = dstScale;
		return this;
	}

	glm::quat GetQuaternion() {
		// return glm::quat(glm::vec3(glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z)));
		// return glm::quat(glm::vec3(rotation.x, rotation.y, rotation.z));
		return glm::quat(glm::radians(rotation));
	}

	void Translate(const glm::vec3& translation) { position += translation; }
};

struct Model {
	Shared<MeshModel> model;
	Transform offset{glm::vec3(0.0f)};
	bool enable = true;

	Model()             = delete;
	Model(const Model&) = default;
	Model(std::string_view path)
		: model(MeshModel::CreateMeshModelFromFile(path)) {}
	void SetOffsetPosition(const glm::vec3& pos) { offset.position = pos; }
	void SetOffsetRotation(const glm::vec3& rot) { offset.rotation = rot; }
	void SetOffetScale(const glm::vec3& scale) { offset.scale = scale; }
};

struct Animation {
	Unique<Animations> animationList;

	Animation(std::string_view path)
		: animationList(Animations::LoadAnimationsFromFile(path)) {}
};

struct Camera {
	Rava::Camera view;
	bool mainCamera      = false;
	bool fixedAspect     = false;
	bool smoothTranslate = true;

	Camera()
		: Camera(false) {}
	Camera(bool isCurrentCamera)
		: mainCamera(isCurrentCamera) {}
};

struct PointLight {
	glm::vec3 color      = {1.0f, 1.0f, 1.0f};
	float lightIntensity = 0.2f;
	float radius         = 0.1f;

	PointLight()
		: PointLight({1.0f, 1.0f, 1.0f}){};
	PointLight(glm::vec3 col, float intensity = 0.2f, float radius = 0.1f) {
		color          = col;
		lightIntensity = intensity;
		this->radius   = radius;
	}
};

struct DirectionalLight {
	glm::vec3 color      = {1.0f, 1.0f, 1.0f};
	float lightIntensity = 1.0f;
	// glm::vec3 direction     = {-1.0f, -3.0f, -1.0f};
	// Rava::Camera* lightView = nullptr;
	// int renderPass          = 0;

	DirectionalLight()
		: DirectionalLight({1.0f, 1.0f, 1.0f}){};
	DirectionalLight(glm::vec3 col, float intensity = 0.2f, glm::vec3 dir = {-1.0f, -3.0f, -1.0f}) {
		color          = col;
		lightIntensity = intensity;
		// direction      = dir;
	}
};

struct RigidBody {
	UniquePx<physx::PxRigidActor> actor = nullptr;
	Transform offset{glm::vec3(0.0f)};
	physx::PxMaterial* material = nullptr;
	bool useDefaultMaterial     = false;

	RigidBody()                 = default;
	RigidBody(const RigidBody&) = default;
	RigidBody(
		Entity& entity,
		PhysicsSystem::ColliderType colliderType,
		bool isTrigger                = false,
		bool isDynamic                = false,
		physx::PxMaterial* pxMaterial = nullptr
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
			actor->userData = reinterpret_cast<void*>(entity.GetEntityID());
		} else {
			actor           = WrapUnique(Engine::s_Instance->GetPhysicsSystem().GetPhysics().createRigidStatic(localTm));
			actor->userData = reinterpret_cast<void*>(entity.GetEntityID());
		}

		auto flags =
			physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE;
		if (isTrigger) {
			flags = physx::PxShapeFlag::eTRIGGER_SHAPE;
		}

		glm::vec3 lower{}, upper{};
		float width   = 1.0f;
		bool hasModel = entity.HasComponent<Component::Model>();
		if (hasModel) {
			lower = entity.GetComponent<Model>()->model->GetBounds().lower;
			upper = entity.GetComponent<Model>()->model->GetBounds().upper;
			width = entity.GetComponent<Model>()->model->GetWidth();
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
				physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxSphereGeometry(width / 2.0f), &material, flags);
				break;
			}
			case PhysicsSystem::ColliderType::Capsule: {
				if (hasModel) {
					physx::PxRigidActorExt::createExclusiveShape(
						*actor, physx::PxCapsuleGeometry(width / 2.0f, upper.y - lower.y - width), &material, flags
					);
				} else {
					physx::PxRigidActorExt::createExclusiveShape(
						*actor, physx::PxCapsuleGeometry(width / 2.0f, 1.0f), &material, flags
					);
				}
				break;
			}
			case PhysicsSystem::ColliderType::Plane: {
				auto shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxPlaneGeometry(), &material, flags);
				shape->setLocalPose(physx::PxTransformFromPlaneEquation(physx::PxPlane(0.0f, 1.0f, 0.0f, 10.0f)));
				break;
			}
			case PhysicsSystem::ColliderType::ConvexMesh: {
				const auto convexMesh =
					Engine::s_Instance->GetPhysicsSystem().CreateConvexMesh(*entity.GetComponent<Model>()->model);
				physx::PxConvexMeshGeometry convexGeometry(convexMesh, physx::PxMeshScale(ToVec3(entity.GetScale())));
				physx::PxRigidActorExt::createExclusiveShape(*actor, convexGeometry, &material, flags);
				break;
			}
			case PhysicsSystem::ColliderType::TriangleMesh: {
				const auto triangleMesh =
					Engine::s_Instance->GetPhysicsSystem().CreateTriangleMesh(*entity.GetComponent<Model>()->model);
				physx::PxTriangleMeshGeometry meshGeometry(triangleMesh, physx::PxMeshScale(ToVec3(entity.GetScale())));
				physx::PxRigidActorExt::createExclusiveShape(*actor, meshGeometry, &material, flags);
				break;
			}
		}
	}
	~RigidBody() {
		if (!useDefaultMaterial) {
			material->release();
		}

		Engine::s_Instance->GetCurrentPxScene()->removeActor(*actor);
	}

	void UpdateMassAndInertia(float mass) const {
		physx::PxRigidBody* rigidBody = reinterpret_cast<physx::PxRigidBody*>(actor.get());
		if (rigidBody) {
			physx::PxRigidBodyExt::updateMassAndInertia(*rigidBody, mass);
		}
	}
};

}  // namespace Rava::Component