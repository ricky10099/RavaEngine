#pragma once

#include <type_traits>

// #include "Framework/Components.h"

namespace Rava {
namespace Component {
struct Name;
struct Transform;
struct RigidBody;
}  // namespace Component

class Scene;
class Entity {
   public:
	Entity() = delete;
	Entity(entt::entity entity, Scene* scene, std::string_view name = "Empty Entity");
	Entity(const Entity& other) = default;
	~Entity()                   = default;

	virtual void Init(){};
	virtual void Update(){};

	virtual void OnTriggerEnter(Entity* other) {}
	virtual void OnTriggerExit(Entity* other) {}
	virtual void OnContactEnter(Entity* other);
	virtual void OnContactStay(Entity* other) {}
	virtual void OnContactExit(Entity* other) {}

	void SetName(std::string_view name);

	void Translate(const glm::vec3& translation);
	void SetPosition(const glm::vec3& position);
	void SetRotation(const glm::vec3& rotation);
	void SetScale(const glm::vec3& scale);

	glm::vec3 GetPosition();
	glm::vec3 GetRotation();
	glm::vec3 GetScale();

	entt::entity GetEntityID() const { return m_entity; }
	std::string GetName() const;


	template <typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		ENGINE_ASSERT(!HasComponent<T>(), "Entity already has component!");
		return &m_scene->GetRegistry().emplace<T>(m_entity, std::forward<Args>(args)...);
	}

	template <typename... Args>
	Component::RigidBody* AddRigidBody(Args&&... args) {
		return AddComponent<Component::RigidBody>(*this, std::forward<Args>(args)...);
	}


	template <typename T>
	T* GetComponent() {
		ENGINE_ASSERT(HasComponent<T>(), "Entity does not have component!");
		return &m_scene->GetRegistry().get<T>(m_entity);
	}

	template <typename T>
	void RemoveComponent() {
		ENGINE_ASSERT(HasComponent<T>(), "Entity does not have component!");
		m_scene->GetRegistry().remove<T>(m_entity);
	}

	template <typename T>
	bool HasComponent() {
		return m_scene->GetRegistry().all_of<T>(m_entity);
	}

   protected:
	// const Entity* m_parent;
	// std::vector<std::shared_ptr<Entity>> m_children;

	entt::entity m_entity{entt::null};
	Scene* m_scene                    = nullptr;
	Component::Name* m_name           = nullptr;
	Component::Transform* m_transform = nullptr;

	bool m_isVisible = true;
};
}  // namespace Rava