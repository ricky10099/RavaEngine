#include "ravapch.h"

#include "Framework/Scene.h"
#include "Framework/Entity.h"

namespace Rava {
Entity::Entity(entt::entity entity, Scene* scene, std::string_view name)
	: m_entity(entity)
	, m_scene(scene)
	, m_name(name) {
	m_transform = AddComponent<Component::Transform>();
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
}  // namespace Rava