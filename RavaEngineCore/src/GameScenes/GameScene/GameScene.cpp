#include "ravapch.h"

#include "GameScene.h"
#include "Framework/Components.h"
#include "Framework/Input.h"
#include "Framework/Timestep.h"
#include "Framework/PhysicsSystem.h"
#include "Entities/Player.h"

void GameScene::Init() {
	m_player = CreateEntity<Player>("Player");

	m_field = CreateEntity<Rava::Entity>("Field");
	m_field->AddComponent<Rava::Component::Model>("Assets/Models/Field/Field.obj");
	m_field->AddRigidBody(Rava::PhysicsSystem::ColliderType::TriangleMesh, false, false);
	m_tree = CreateEntity<Rava::Entity>("Tree");
	m_tree->AddComponent<Rava::Component::Model>("Assets/Models/Tree/Tree.obj");
	m_tree->AddRigidBody(Rava::PhysicsSystem::ColliderType::Box, false, false);
	m_directionalLight = CreateEntity<Rava::Entity>("Directional Light");
	m_directionalLight->AddComponent<Rava::Component::DirectionalLight>();
}

void GameScene::Update() {}
