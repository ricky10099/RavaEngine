#include "ravapch.h"

#include "ExampleScene.h"
#include "Framework/Components.h"
#include "Framework/Input.h"
#include "Framework/Timestep.h"
#include "Framework/PhysicsSystem.h"

void ExampleScene::Init() {
	std::vector<glm::vec3> lightColors{
		{1.0f, 0.1f, 0.1f},
		{0.1f, 0.1f, 1.0f},
		{0.1f, 1.0f, 0.1f},
		{1.0f, 1.0f, 0.1f},
		{0.1f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
	};

	for (int i = 0; i < lightColors.size(); i++) {
		std::string name = "Point Light " + std::to_string(i);
		m_pointLights[i] = CreateEntity(name);
		m_pointLights[i]->AddComponent<Rava::Component::PointLight>(lightColors[i]);
		auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), {0.0f, 1.0f, 0.0f});
		m_pointLights[i]->SetPosition(glm::vec3(rotateLight * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)));
	}

	m_entity = CreateEntity("Test");
	m_entity->AddComponent<Rava::Component::Model>("Assets/Models/Male.obj");
	m_entity->GetComponent<Rava::Component::Transform>()->SetScale(glm::vec3{0.5f})->SetPosition(glm::vec3(0.0f, 0.5f, 0.0f));

	m_testLight = CreateEntity("testLight");
	m_testLight->AddComponent<Rava::Component::PointLight>(glm::vec3(1.f, 0.f, 0.f), 0.1f, 0.1f);
	m_testLight->AddRigidBody(Rava::PhysicsSystem::ColliderType::Box, false, true);
	m_testLight->GetComponent<Rava::Component::RigidBody>()->UpdateMassAndInertia(0.01f);

	m_entity2 = CreateEntity("Fish");
	m_entity2->AddComponent<Rava::Component::Model>("Assets/Models/Fish/Fish.fbx");
	//m_entity2->AddComponent<Rava::Component::Animation>("Assets/Models/Fish/Fish.fbx");
	m_entity2->SetScale(glm::vec3{0.01f});

	m_entity3 = CreateEntity("Dragon");
	m_entity3->AddComponent<Rava::Component::Model>("Assets/Models/Dragon/M_B_44_Qishilong_skin_Skeleton.fbx");
	//m_entity3->AddComponent<Rava::Component::Animation>("Assets/Models/Dragon/M_B_44_Qishilong_skin_Skeleton.fbx");
	m_entity3->SetScale(glm::vec3{0.001f});
}

void ExampleScene::Update() {
	// if (Input::IsKeyPress(Key::W)) {
	//	m_debugCamera->Translate(glm::vec3{0.0f, 0.0f, -5.0f} * Rava::Timestep::Count());
	// }
	// if (Input::IsKeyPress(Key::S)) {
	//	m_debugCamera->Translate(glm::vec3{0.0f, 0.0f, 5.0f} * Rava::Timestep::Count());
	// }
	// if (Input::IsKeyPress(Key::A)) {
	//	m_debugCamera->Translate(glm::vec3{-5.0f, 0.0f, 0.0f} * Rava::Timestep::Count());
	// }
	// if (Input::IsKeyPress(Key::D)) {
	//	m_debugCamera->Translate(glm::vec3{5.0f, 0.0f, 0.0f} * Rava::Timestep::Count());
	// }
	// if (Input::IsKeyPress(Key::Q)) {
	//	m_debugCamera->Translate(glm::vec3{0.0f, 5.0f, 0.0f} * Rava::Timestep::Count());
	// }
	// if (Input::IsKeyPress(Key::E)) {
	//	m_debugCamera->Translate(glm::vec3{0.0f, -5.0f, 0.0f} * Rava::Timestep::Count());
	// }

	if (Input::IsKeyPress(Key::Space)) {
		m_testLight->Translate(glm::vec3{0.0f, 1.0f, 0.0f} * Rava::Timestep::Count());
	}

	auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * Rava::Timestep::Count(), {0.f, -1.f, 0.f});

	for (int i = 0; i < m_pointLights.size(); i++) {
		m_pointLights[i]->SetPosition(glm::vec3(rotateLight * glm::vec4(m_pointLights[i]->GetPosition(), 1.f)));
	}
}

void ExampleScene::ClearRegistry() {
	LOG_TRACE("Clear");
}
