#pragma once

#include "Framework/Components.h"

namespace Rava {
class Scene;
class Entity {
   public:
	Entity() = delete;
	Entity(entt::entity entity, Scene* scene, std::string_view name = "Empty Entity");
	Entity(const Entity& other) = default;

	virtual void Init(){};
	virtual void Update(){};

	void SetName(std::string_view name) { m_name = name; }

	void Translate(const glm::vec3& translation);
	void SetPosition(const glm::vec3& position);
	void SetRotation(const glm::vec3& rotation);
	void SetScale(const glm::vec3& scale);

	glm::vec3 GetPosition() { return m_transform->position; }
	glm::vec3 GetRotation() { return m_transform->rotation; }
	glm::vec3 GetScale() { return m_transform->scale; }

	entt::entity GetEntityID() const { return m_entity; }
	std::string_view GetName() const { return m_name; }

	template <typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		ENGINE_ASSERT(!HasComponent<T>(), "Entity already has component!");
		return &m_scene->GetRegistry().emplace<T>(m_entity, std::forward<Args>(args)...);
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
	entt::entity m_entity{entt::null};
	Scene* m_scene = nullptr;
	// const Entity* m_parent;
	// std::vector<std::shared_ptr<Entity>> m_children;
	std::string m_name;
	// glm::vec3 m_position{0.f, 0.f, 0.f};
	Component::Transform* m_transform{};

	bool m_isVisible = true;
};
}  // namespace Rava