#include "ravapch.h"

#include "Framework/Scene.h"
#include "Framework/Entity.h"
#include "Framework/Components.h"

namespace Rava {
//Scene::Scene(std::string_view name)
//	: m_name(name) {
//	m_debugCamera = CreateEntity("Debug Camera");
//	m_debugCamera->AddComponent<Component::Camera>(true);
//	m_debugCamera->GetComponent<Component::Transform>()->SetPosition(glm::vec3{0.0f, 0.5f, 5.0f});
//
//	m_directionalLight = CreateEntity("Directional Light");
//	m_directionalLight->AddComponent<Component::DirectionalLight>();
//}

Shared<Entity> Scene::CreateEntity(std::string_view name) {
	Shared<Entity> entity = std::make_shared<Entity>(m_registry.create(), this, name);
	m_entities.push_back(entity);
	return entity;
}

void Scene::DestroyEntity(u32 index) {
	m_registry.destroy(m_entities[index]->GetEntityID());
	m_entities.erase(m_entities.begin() + index);
}
}  // namespace Rava