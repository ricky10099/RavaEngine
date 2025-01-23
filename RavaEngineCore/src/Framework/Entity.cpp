#include "ravapch.h"

#include "Framework/Scene.h"
#include "Framework/Entity.h"
#include "Framework/Components.h"

namespace Rava {
Entity::Entity(entt::entity entity, Scene* scene, std::string_view name)
	: m_entity(entity)
	, m_scene(scene)
/*, m_name(name)*/ {
	m_name      = AddComponent<Component::Name>(name);
	m_transform = AddComponent<Component::Transform>();
}

//template <typename T, typename... Args>
//T* Entity::AddComponent(Args&&... args) {
//	ENGINE_ASSERT(!HasComponent<T>(), "Entity already has component!");
//
//	//// Check if the component's constructor accepts an Entity
//	//if constexpr (std::is_constructible_v<T, Entity&, Args...>) {
//	//	// Pass the current Entity instance to the component
//	//	return &m_scene->GetRegistry().emplace<T>(m_entity, *this, std::forward<Args>(args)...);
//	//} else {
//	//	// Construct without passing the Entity
//	//	return &m_scene->GetRegistry().emplace<T>(m_entity, std::forward<Args>(args)...);
//	//}
//
//	 return &m_scene->GetRegistry().emplace<T>(m_entity, std::forward<Args>(args)...);
//}


void Entity::SetName(std::string_view name) {
	m_name->SetName(name);
}
void Entity::Translate(const glm::vec3& translation) {
	m_transform->Translate(translation);
}

void Entity::SetPosition(const glm::vec3& position) {
	m_transform->SetPosition(position);
}

void Entity::SetRotation(const glm::vec3& rotation) {
	m_transform->SetRotation(rotation);
}

void Entity::SetScale(const glm::vec3& scale) {
	m_transform->SetScale(scale);
}

glm::vec3 Entity::GetPosition() {
	return m_transform->position;
}

glm::vec3 Entity::GetRotation() {
	return m_transform->rotation;
}

glm::vec3 Entity::GetScale() {
	return m_transform->scale;
}

std::string Entity::GetName() const {
	return m_name->data;
}
}  // namespace Rava