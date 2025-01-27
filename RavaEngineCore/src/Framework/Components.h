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
		return glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::quat(rotation)) * glm::scale(glm::mat4(1.0f), scale);
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

	glm::quat GetQuaternion() { return glm::quat(rotation); }

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
	Transform offset{glm::vec3(0.0f)};
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

	DirectionalLight()
		: DirectionalLight({1.0f, 1.0f, 1.0f}){};
	DirectionalLight(glm::vec3 col, float intensity = 0.2f, glm::vec3 dir = {-1.0f, -3.0f, -1.0f}) {
		color          = col;
		lightIntensity = intensity;
	}
};

struct RigidBody {
	UniquePx<physx::PxRigidActor> actor = nullptr;
	Transform offset{glm::vec3(0.0f)};
	physx::PxMaterial* material = nullptr;
	bool useDefaultMaterial     = false;

	RigidBody()                 = delete;
	RigidBody(const RigidBody&) = default;
	RigidBody(
		Entity& entity,
		PhysicsSystem::ColliderType colliderType,
		bool isTrigger                = false,
		bool isDynamic                = false,
		physx::PxMaterial* pxMaterial = nullptr
	);
	~RigidBody();

	void UpdateMassAndInertia(float mass) const {
		if (actor->getType() == physx::PxActorType::eRIGID_DYNAMIC) {
			physx::PxRigidDynamic* rigidBody = reinterpret_cast<physx::PxRigidDynamic*>(actor.get());
			physx::PxRigidBodyExt::updateMassAndInertia(*rigidBody, mass);
		}
	}
};

}  // namespace Rava::Component