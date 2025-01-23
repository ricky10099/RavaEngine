#include "ravapch.h"

#include "Framework/Scene.h"
#include "Framework/Entity.h"
#include "Framework/Components.h"

namespace Rava {
Shared<Entity> Scene::CreateEntity(std::string_view name) {
	Shared<Entity> entity = std::make_shared<Entity>(m_registry.create(), this, name);
	m_entities.push_back(entity);
	return entity;
}

void Scene::DestroyEntity(u32 index) {
	m_registry.destroy(m_entities[index]->GetEntityID());
	m_entities.erase(m_entities.begin() + index);
}
void Scene::CreatePhysXScene() {
	m_pxScene = Engine::s_Instance->GetPhysicsSystem().CreatePhysXScene();
}
}  // namespace Rava