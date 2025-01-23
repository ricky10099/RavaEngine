#pragma once

namespace Rava {
// PhysX classes' destructor are protected
template <typename T>
struct Deleter {
	void operator()(T* p) {
		if (p) {
			p->release();
		}
	}
};
template<typename T>
struct deleter_type {
    using type = std::conditional_t<std::is_base_of_v<physx::PxBase, T>, physx::PxBase, T>;
};
template<typename T>
using deleter_type_t = typename deleter_type<T>::type;

template<typename T, typename D = Deleter<deleter_type_t<T>>>
using UniquePx = std::unique_ptr<T, D>;

template <typename T, typename D = Deleter<deleter_type_t<T>>>
UniquePx<T, D> WrapUnique(T* p) {
	return UniquePx<T, D>(p);
}

/// Convert Raygun Vec3 to PhysX Vec3.
static inline const physx::PxVec3& ToVec3(const glm::vec3& v) {
	return reinterpret_cast<const physx::PxVec3&>(v);
}

/// Convert PhysX Vec3 to Raygun Vec3.
static inline const glm::vec3& ToVec3(const physx::PxVec3& v) {
	return reinterpret_cast<const glm::vec3&>(v);
}

/// Convert Raygun Quaternion to PhysX Quaternion.
static inline const physx::PxQuat& ToQuat(const glm::quat& q) {
	return reinterpret_cast<const physx::PxQuat&>(q);
}

/// Convert PhysX Quaternion to Raygun Quaternion.
static inline const glm::quat& ToQuat(const physx::PxQuat& q) {
	return reinterpret_cast<const glm::quat&>(q);
}

/// Convert Raygun transform to PhysX transform.
static inline physx::PxTransform ToTransform(const glm::vec3& t, const glm::quat& q) {
	return physx::PxTransform{ToVec3(t), ToQuat(q)};
}

static inline std::set<physx::PxActor*> GetActors(physx::PxScene& scene) {
	const auto typeFlags = physx::PxActorTypeFlag::eRIGID_DYNAMIC | physx::PxActorTypeFlag::eRIGID_STATIC;

	const auto count = scene.getNbActors(typeFlags);

	std::vector<physx::PxActor*> actors(count);
	scene.getActors(typeFlags, actors.data(), (physx::PxU32)actors.size());

	return {actors.begin(), actors.end()};
}

static inline UniquePx<physx::PxMaterial> CloneMaterial(physx::PxPhysics& physics, const physx::PxMaterial& material) {
	const auto staticFriction  = material.getStaticFriction();
	const auto dynamicFriction = material.getStaticFriction();
	const auto restitution     = material.getRestitution();

	auto result = WrapUnique(physics.createMaterial(staticFriction, dynamicFriction, restitution));
	result->setBaseFlags(material.getBaseFlags());
	result->setFlags(material.getFlags());

	return result;
}

}  // namespace Rava