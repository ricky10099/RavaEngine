#pragma once

#include "Framework/Scene.h"
#include "Framework/Entity.h"

class Player;

class GameScene : public Rava::Scene {
   public:
	GameScene()
		: Rava::Scene("Game Scene") {}
	~GameScene() = default;

	virtual void Init() override;
	virtual void Update() override;

   private:
	Shared<Player> m_player;
	Shared<Rava::Entity> m_field;
	Shared<Rava::Entity> m_tree;
	Shared<Rava::Entity> m_directionalLight;
};