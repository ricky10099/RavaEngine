#pragma once

#include "Framework/Scene.h"
#include "Framework/Entity.h"

class ExampleScene : public Rava::Scene {
   public:
	ExampleScene()
		: Rava::Scene("Example Scene") {}
	~ExampleScene() = default;

	virtual void Init() override;
	virtual void Update() override;

   private:
	Shared<Rava::Entity> m_entity;
	Shared<Rava::Entity> m_entity2;
	Shared<Rava::Entity> m_entity3;
	Shared<Rava::Entity> m_testLight;

	std::array<Shared<Rava::Entity>, 6> m_pointLights;
};