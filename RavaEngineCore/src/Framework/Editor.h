#pragma once

#include "Framework/Scene.h"
#include "Framework/Vulkan/SwapChain.h"

namespace Rava {
class Editor {
   public:
	Editor(VkRenderPass renderPass, u32 imageCount);
	~Editor();

	NO_COPY(Editor)

	void NewFrame();
	void Render(VkCommandBuffer commandBuffer);
	void Organize(Scene* scene, u32 currentFrame);
	void InputHandle();

	void RecreateDescriptorSet(VkImageView swapChainImage, u32 imageCount);
	
	void Reset() {
		m_selectedEntity = nullptr;
		m_gizmoType      = -1;
	}

	void SetSelectedEntity(Shared<Entity> entity) { m_selectedEntity = entity; }

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 70.0f);

	template <typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Shared<Entity> entity, UIFunction uiFunction, bool removable = true);

   private:
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	VkSampler m_textureSampler = VK_NULL_HANDLE;

	bool m_viewportFocused = false, m_viewportHovered = false;

	glm::vec2 m_viewportSize = {0.0f, 0.0f};
	// glm::vec2 m_viewportBounds[2];
	Shared<Entity> m_selectedEntity = nullptr;
	u32 m_selectedIndex             = -1;
	int m_gizmoType                 = -1;



   private:
	void DrawSceneHierarchy(Scene* scene);
	void DrawGizmo();
	bool DrawEntityNode(Scene* scene, const Shared<Entity>& entity, u32 index);
	void DrawComponents(Shared<Entity> entity);
	template <typename T>
	void DisplayAddComponentEntry(const std::string& entryName);
	template <typename T>
	void DisplayAddComponentFromFile(const std::string& entryName);
	void SetEditorStlye();
	void SetEditorThemeColors();
};
}  // namespace Rava6