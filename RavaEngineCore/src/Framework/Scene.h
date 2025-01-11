#pragma once

namespace Rava {
class Entity;
class Scene {
	friend class Entity;
   public:
	//Scene():;
	Scene(std::string_view name = "Scene");
	virtual ~Scene() = default;

	virtual void Init(){};
	virtual void Update(){};
	virtual void Exit(){};

	Shared<Entity> CreateEntity(std::string_view name);
	void DestroyEntity(u32 index);
	std::string_view GetName() const { return m_name; }
	entt::registry& GetRegistry() { return m_registry; };
	const Shared<Entity>& GetEntity(u32 index) { return m_entities[index]; }
	size_t GetEntitySize() { return m_entities.size(); }

   protected:
	std::string_view m_name;
	entt::registry m_registry;
	Shared<Entity> m_debugCamera;
	Shared<Entity> m_directionalLight;
	std::vector<Shared<Entity>> m_entities;

	// bool m_isRunning;

	// u32 m_sceneLightsGroupNode = 0;
	// u32 m_lightCounter         = 0;
};
}  // namespace Rava