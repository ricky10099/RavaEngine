#include "ravapch.h"

#include "Player.h"
#include "Framework/Components.h"
#include "Framework/Input.h"

void Player::Init() {
	m_model = AddComponent<Rava::Component::Model>("Assets/Models/Tokage/tokage.obj");
	m_rigidBody = AddRigidBody(Rava::PhysicsSystem::ColliderType::Box, false, true);
	m_rigidBody->UpdateMassAndInertia(1.0f);
	m_camera = AddComponent<Rava::Component::Camera>(true);
}

void Player::Update() {
	if (Input::IsKeyPress(Key::W)) {
		Translate(glm::vec3{0.0f, 0.0f, -5.0f} * Rava::Timestep::Count());
	}
	if (Input::IsKeyPress(Key::S)) {
		Translate(glm::vec3{0.0f, 0.0f, 5.0f} * Rava::Timestep::Count());
	}
	if (Input::IsKeyPress(Key::A)) {
		Translate(glm::vec3{-5.0f, 0.0f, 0.0f} * Rava::Timestep::Count());
	}
	if (Input::IsKeyPress(Key::D)) {
		Translate(glm::vec3{5.0f, 0.0f, 0.0f} * Rava::Timestep::Count());
	}
}

