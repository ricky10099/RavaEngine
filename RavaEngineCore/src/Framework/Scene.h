#pragma once
namespace Rava {
class Scene {
   public:
	Scene() {}
	~Scene() = default;

	void Init() { LOG_TRACE("Scene Init()"); }

	void Update() {}
};
}  // namespace Rava