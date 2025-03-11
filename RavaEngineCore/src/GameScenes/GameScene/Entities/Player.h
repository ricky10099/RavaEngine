#pragma once

#include "Framework/Entity.h"

namespace Rava::Component{
struct Model;
struct Camera;
struct RigidBody;
}

class Player : public Rava::Entity {
   public:
	Player(entt::entity entity, Rava::Scene* scene, std::string_view name)
		: Rava::Entity(entity, scene, name) {}

	virtual void Init() override;
	virtual void Update() override;

   private:
	Rava::Component::Model* m_model         = nullptr;
	Rava::Component::Camera* m_camera       = nullptr;
	Rava::Component::RigidBody* m_rigidBody = nullptr;
};