#pragma once

namespace physx {
class PxScene;
}

namespace Rava {
class Entity;
class Scene {
	friend class Editor;
	friend class Engine;

   public:
	Scene() = delete;
	Scene(std::string_view name) {}
	~Scene() = default;

	virtual void Init(){};
	virtual void Update(){};
	virtual void Exit(){};

	template <typename T>
	Shared<T> CreateEntity(std::string_view name) {
		static_assert(std::is_base_of_v<Entity, T>, "T must be derived from Entity");

		Shared<T> entity = std::make_shared<T>(m_registry.create(), this, name);
		m_entities.push_back(entity);
		return entity;
	}

	void DestroyEntity(u32 index);

	std::string_view GetName() const { return m_name; }
	entt::registry& GetRegistry() { return m_registry; };
	std::vector<Shared<Entity>>& GetAllEntities() { return m_entities; }
	const Shared<Entity>& GetEntity(u32 index) { return m_entities[index]; }
	size_t GetEntitySize() { return m_entities.size(); }
	physx::PxScene* GetPxScene() { return m_pxScene; }

   protected:
	std::string_view m_name = typeid(*this).name();
	entt::registry m_registry;
	physx::PxScene* m_pxScene = nullptr;
	std::vector<Shared<Entity>> m_entities;

   private:
	void CreatePhysXScene();

	void ClearScene() {
		m_registry.clear();
		m_pxScene->release();
		m_entities.clear();
	}

	// bool m_isRunning;

	// u32 m_sceneLightsGroupNode = 0;
	// u32 m_lightCounter         = 0;
};
}  // namespace Rava