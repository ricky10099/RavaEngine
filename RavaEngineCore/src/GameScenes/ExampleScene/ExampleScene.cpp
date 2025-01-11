#include "ravapch.h"

#include "ExampleScene.h"
#include "Framework/Components.h"
#include "Framework/Input.h"
#include "Framework/Timestep.h"

ExampleScene::ExampleScene(std::string_view name)
	: Scene(name) {
	m_entity = CreateEntity("Test");
	m_entity->AddComponent<Rava::Component::Model>("Models/Male.obj");
	m_entity->GetComponent<Rava::Component::Transform>()->SetScale(glm::vec3{0.5f})->SetPosition(glm::vec3(0.0f, 0.5f, 0.0f));

	m_testLight = CreateEntity("testLight");
	m_testLight->AddComponent<Rava::Component::PointLight>(glm::vec3(1.f, 0.f, 0.f), 0.1f, 0.1f);
}

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
		std::string name = " Point Light " + std::to_string(i);
		auto pointLight       = CreateEntity(name);
		pointLight->AddComponent<Rava::Component::PointLight>(lightColors[i]);
		auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), {0.0f, 1.0f, 0.0f});
		pointLight->GetComponent<Rava::Component::Transform>()->SetPosition(
			glm::vec3(rotateLight * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
		);
	}
}

void ExampleScene::Update() {
	if (Input::IsKeyPress(Key::W)) {
		m_debugCamera->Translate(glm::vec3{0.0f, 0.0f, -5.0f} * Rava::Timestep::Count());
	}
	if (Input::IsKeyPress(Key::S)) {
		m_debugCamera->Translate(glm::vec3{0.0f, 0.0f, 5.0f} * Rava::Timestep::Count());
	}
	if (Input::IsKeyPress(Key::A)) {
		m_debugCamera->Translate(glm::vec3{-5.0f, 0.0f, 0.0f} * Rava::Timestep::Count());
	}
	if (Input::IsKeyPress(Key::D)) {
		m_debugCamera->Translate(glm::vec3{5.0f, 0.0f, 0.0f} * Rava::Timestep::Count());
	}
	if (Input::IsKeyPress(Key::Q)) {
		m_debugCamera->Translate(glm::vec3{0.0f, 5.0f, 0.0f} * Rava::Timestep::Count());
	}
	if (Input::IsKeyPress(Key::E)) {
		m_debugCamera->Translate(glm::vec3{0.0f, -5.0f, 0.0f} * Rava::Timestep::Count());
	}

	auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * Rava::Timestep::Count(), {0.f, -1.f, 0.f});
	auto view = m_registry.view<Rava::Component::PointLight, Rava::Component::Transform>();
	for (auto entity : view) {
		auto& pointLight = view.get<Rava::Component::PointLight>(entity);
		auto& transform  = view.get<Rava::Component::Transform>(entity);
		transform.position = glm::vec3(rotateLight * glm::vec4(transform.position, 1.f));
	}
}