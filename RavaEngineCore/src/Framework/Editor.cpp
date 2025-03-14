#include "ravapch.h"

#include "Framework/Editor.h"
#include "Framework/RavaUtils.h"
#include "Framework/Vulkan/VKUtils.h"
#include "Framework/RavaEngine.h"
#include "Framework/Vulkan/Renderer.h"
#include "Framework/Components.h"
#include "Framework/Entity.h"
#include "Framework/Camera.h"
#include "Framework/Input.h"
#include "Framework/Window.h"

namespace Rava {

Editor::Editor(VkRenderPass renderPass, u32 imageCount) {
	// set up a descriptor pool stored on this instance, see header for more comments on this.
	VkDescriptorPoolSize pool_sizes[] = {
		{			   VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		{		 VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		{		 VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		{  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		{  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		{        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		{      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets                    = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount              = static_cast<u32>(IM_ARRAYSIZE(pool_sizes));
	pool_info.pPoolSizes                 = pool_sizes;

	VkResult result = vkCreateDescriptorPool(VKContext->GetLogicalDevice(), &pool_info, nullptr, &m_descriptorPool);
	VK_CHECK(result, "Failed to set up Descriptor pool for ImGui");

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform Windows

	float fontSize = 18.0f;
	io.Fonts->AddFontFromFileTTF(
		"Assets/System/Fonts/GenJyuuGothic/GenJyuuGothicX-Monospace-Regular.ttf",
		fontSize,
		nullptr,
		io.Fonts->GetGlyphRangesChineseFull()
	);
	io.Fonts->AddFontFromFileTTF(
		"Assets/System/Fonts/GenJyuuGothic/GenJyuuGothicX-Monospace-Light.ttf",
		fontSize,
		nullptr,
		io.Fonts->GetGlyphRangesChineseFull()
	);
	io.FontDefault = io.Fonts->AddFontFromFileTTF(
		"Assets/System/Fonts/GenJyuuGothic/GenJyuuGothicX-Monospace-Bold.ttf",
		fontSize,
		nullptr,
		io.Fonts->GetGlyphRangesChineseFull()
	);

	SetEditorStlye();
	SetEditorThemeColors();

	// Setup Platform/Renderer backends
	// Initialize imgui for vulkan
	ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)Engine::s_Instance->GetGLFWWindow(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance                  = VKContext->GetInstance();
	init_info.PhysicalDevice            = VKContext->GetPhysicalDevice();
	init_info.Device                    = VKContext->GetLogicalDevice();
	init_info.QueueFamily               = VKContext->GetPhysicalQueueFamilies().graphicsFamily;
	init_info.Queue                     = VKContext->GetGraphicsQueue();
	init_info.PipelineCache             = VK_NULL_HANDLE;
	init_info.DescriptorPool            = m_descriptorPool;
	init_info.RenderPass                = renderPass;
	init_info.Subpass                   = 0;
	init_info.Allocator                 = VK_NULL_HANDLE;
	init_info.MinImageCount             = 2;
	init_info.ImageCount                = imageCount;
	init_info.CheckVkResultFn           = CheckVKResult;
	ImGui_ImplVulkan_Init(&init_info);

	m_descriptorSets.resize(imageCount);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter               = VK_FILTER_LINEAR;
	samplerInfo.minFilter               = VK_FILTER_LINEAR;
	samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable        = VK_FALSE;
	samplerInfo.maxAnisotropy           = 1.0f;
	samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable           = VK_FALSE;
	samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias              = 0.0f;
	samplerInfo.minLod                  = 0.0f;
	samplerInfo.maxLod                  = 0.0f;

	result = vkCreateSampler(VKContext->GetLogicalDevice(), &samplerInfo, nullptr, &m_textureSampler);
	VK_CHECK(result, "failed to create texture sampler!");
}

Editor::~Editor() {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(VKContext->GetLogicalDevice(), m_descriptorPool, nullptr);
	vkDestroySampler(VKContext->GetLogicalDevice(), m_textureSampler, nullptr);
}

void Editor::NewFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
}

void Editor::Render(VkCommandBuffer commandBuffer) {
	ImGui::Render();
	ImDrawData* drawdata = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawdata, commandBuffer);

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void Editor::Organize(Scene* scene, u32 currentFrame) {
	DrawSceneHierarchy(scene);
	DrawGizmo();

	ImGui::SetNextWindowSize(ImVec2{400.0f, 100.0f}, ImGuiCond_Once);
	ImGui::Begin("Render Setting");
	ImGui::ColorEdit3("Clear Color", (float*)&Engine::s_Instance->clearColor);
	ImGui::DragFloat("Gamma", &Engine::s_Instance->m_gamma, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("Exposure", &Engine::s_Instance->m_exposure, 0.1f, 0.0f, 10.0f);
	ImGui::End();

	ImGui::ShowDemoWindow();

	//    ImGui::Begin("Vulkan Viewport");
	// ImVec2 windowSize = ImGui::GetContentRegionAvail();
	//	ImGui::Image((ImTextureID)m_descriptorSets[currentFrame], windowSize);
	// ImGui::End();
}

void Editor::InputHandle() {
	if (!Input::IsMouseButtonPress(Mouse::ButtonRight)) {
		if (Input::IsKeyDown(Key::Q)) {
			if (!ImGuizmo::IsUsing()) {
				m_gizmoType = -1;
			}
		}
		if (Input::IsKeyDown(Key::W)) {
			if (!ImGuizmo::IsUsing()) {
				m_gizmoType = ImGuizmo::OPERATION::TRANSLATE;
			}
		}
		if (Input::IsKeyDown(Key::E)) {
			if (!ImGuizmo::IsUsing()) {
				m_gizmoType = ImGuizmo::OPERATION::ROTATE;
			}
		}
		if (Input::IsKeyDown(Key::R)) {
			if (!ImGuizmo::IsUsing()) {
				m_gizmoType = ImGuizmo::OPERATION::SCALE;
			}
		}
	}
}

void Editor::RecreateDescriptorSet(VkImageView swapChainImage, u32 currentFrame) {
	m_descriptorSets[currentFrame] =
		ImGui_ImplVulkan_AddTexture(m_textureSampler, swapChainImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void Editor::DrawSceneHierarchy(Scene* scene) {
	ImGui::Begin("Scene Hierarchy");

	for (u32 i = 0; i < scene->GetEntitySize(); ++i) {
		if (DrawEntityNode(scene, scene->GetEntity(i), i)) {
			auto item = scene->m_entities[i];
			if (ImGui::IsItemActive() && !ImGui::IsItemHovered()) {
				int n_next = i + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
				if (n_next >= 0 && n_next < scene->GetEntitySize()) {
					scene->m_entities[i]      = scene->m_entities[n_next];
					scene->m_entities[n_next] = item;
					ImGui::ResetMouseDragDelta();
				}
			}
		} else {
			break;
		}
	}

	if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
		m_selectedEntity = {};
	}

	// Right-click on blank space
	ImGuiPopupFlags flags = ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems;
	if (ImGui::BeginPopupContextWindow(0, flags)) {
		if (ImGui::MenuItem("Create Empty Entity")) {
			scene->CreateEntity<Entity>("Empty Entity");
		}

		ImGui::EndPopup();
	}

	ImGui::End();

	ImGui::Begin("Properties");
	if (m_selectedEntity) {
		DrawComponents(m_selectedEntity);
	}

	ImGui::End();
}

void Editor::DrawGizmo() {
	ImGuizmo::SetOrthographic(false);
	const auto& projectionMatrix = Engine::s_Instance->m_editorCamera.GetProjection();
	auto& viewMatrix             = Engine::s_Instance->m_editorCamera.GetView();
	ImGuiIO& io                  = ImGui::GetIO();
	int windowX, windowY;
	glfwGetWindowPos(Engine::s_Instance->GetGLFWWindow(), &windowX, &windowY);
	ImGuizmo::SetRect(windowX, windowY, io.DisplaySize.x, io.DisplaySize.y);

	if (m_selectedEntity && m_gizmoType != -1) {
		auto& transform =
			Engine::s_Instance->m_currentScene->GetRegistry().get<Component::Transform>(m_selectedEntity->GetEntityID());
		glm::mat4 mat4 = transform.GetTransform();

		bool snap       = Input::IsKeyPress(Key::LeftControl);
		float snapValue = 0.5f;
		if (m_gizmoType == ImGuizmo::OPERATION::ROTATE) {
			snapValue = 5.0f;
		}
		float snapValues[3] = {snapValue, snapValue, snapValue};

		ImGuizmo::Manipulate(
			glm::value_ptr(viewMatrix),
			glm::value_ptr(projectionMatrix),
			(ImGuizmo::OPERATION)m_gizmoType,
			ImGuizmo::LOCAL,
			glm::value_ptr(mat4),
			nullptr,
			snap ? snapValues : nullptr
		);

		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(mat4, scale, rotation, translation, skew, perspective);
		glm::vec3 rotationEuler = glm::eulerAngles(rotation);

		if (ImGuizmo::IsUsing()) {
			transform.SetPosition(translation);
			transform.SetRotation(rotationEuler);
			transform.SetScale(scale);
		}
	}
}

bool Editor::DrawEntityNode(Scene* scene, const Shared<Entity>& entity, u32 index) {
	auto& name = entity->GetComponent<Component::Name>()->data;

	// ImGuiTreeNodeFlags flags = ((m_selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
	// ImGuiTreeNodeFlags_OpenOnArrow;
	//  flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
	ImGui::Selectable(std::string(name + "##" + std::to_string((u32)entity->GetEntityID())).data(), m_selectedIndex == index);
	// bool opened = ImGui::TreeNodeEx((void*)entity.get(), flags, name.data());

	if (ImGui::IsItemClicked()) {
		m_selectedEntity = entity;
		m_selectedIndex  = index;
	}

	bool entityDeleted = false;
	if (ImGui::BeginPopupContextItem()) {
		if (ImGui::MenuItem("Delete Entity")) {
			entityDeleted = true;
		}

		ImGui::EndPopup();
	}

	// if (opened) {
	// ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	// bool opened              = ImGui::TreeNodeEx((void*)9817239, flags, entity->GetName().data());
	// if (opened) {
	//	ImGui::TreePop();
	// }
	// ImGui::TreePop();
	//}

	if (entityDeleted) {
		scene->DestroyEntity(index);
		if (m_selectedEntity == entity) {
			m_selectedEntity = {};
		}
		return false;
	}

	return true;
}

void Editor::DrawComponents(Shared<Entity> entity) {
	if (entity->HasComponent<Component::Name>()) {
		auto& name = entity->GetComponent<Component::Name>()->data;
		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strncpy_s(buffer, sizeof(buffer), name.data(), sizeof(buffer));
		if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
			name = std::string(buffer);
		}
	}
	ImGui::SameLine();
	ImGui::PushItemWidth(-1);

	if (ImGui::Button("Add Component")) {
		ImGui::OpenPopup("Add Component");
	}

	if (ImGui::BeginPopup("Add Component")) {
		DisplayAddComponentEntry<Component::Camera>("Camera");
		DisplayAddComponentEntry<Component::PointLight>("Point Light");
		DisplayAddComponentEntry<Component::DirectionalLight>("Directional Light");
		DisplayAddComponentFromFile<Component::Model>("Model");
		DisplayAddComponentFromFile<Component::Animation>("Animation");
		ImGui::EndPopup();
	}

	ImGui::PopItemWidth();

	DrawComponent<Component::Transform>(
		"Transform",
		entity,
		[](auto& component) {
			DrawVec3Control("Position", component->position);
			glm::vec3 rotation = glm::degrees(component->rotation);
			DrawVec3Control("Rotation", rotation);
			component->rotation = glm::radians(rotation);
			DrawVec3Control("Scale", component->scale, 1.0f);
		},
		false
	);

	DrawComponent<Component::Camera>("Camera", entity, [](auto& component) {
		auto& camera = component->view;

		DrawVec3Control("Offset Position", component->offset.position, 0.0f, 110.0f);
		glm::vec3 rotation = glm::degrees(component->offset.rotation);
		DrawVec3Control("Offset Rotation", rotation, 0.0f, 110.0f);
		component->offset.rotation = glm::radians(rotation);

		ImGui::Checkbox("Main camera", &component->mainCamera);

		const char* projectionTypeStrings[]     = {"Perspective", "Orthographic"};
		const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
		if (ImGui::BeginCombo("Projection", currentProjectionTypeString)) {
			for (int i = 0; i < 2; i++) {
				bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
				if (ImGui::Selectable(projectionTypeStrings[i], isSelected)) {
					currentProjectionTypeString = projectionTypeStrings[i];
					camera.SetProjectionType((Camera::ProjectionType)i);
				}

				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		if (camera.GetProjectionType() == Camera::ProjectionType::Perspective) {
			float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
			if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov)) {
				camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));
			}

			float perspectiveNear = camera.GetPerspectiveNearClip();
			if (ImGui::DragFloat("Near", &perspectiveNear)) {
				camera.SetPerspectiveNearClip(perspectiveNear);
			}

			float perspectiveFar = camera.GetPerspectiveFarClip();
			if (ImGui::DragFloat("Far", &perspectiveFar)) {
				camera.SetPerspectiveFarClip(perspectiveFar);
			}
		}

		if (camera.GetProjectionType() == Camera::ProjectionType::Orthographic) {
			float orthoSize = camera.GetOrthographicSize();
			if (ImGui::DragFloat("Size", &orthoSize)) {
				camera.SetOrthographicSize(orthoSize);
			}

			float orthoNear = camera.GetOrthographicNearClip();
			if (ImGui::DragFloat("Near", &orthoNear)) {
				camera.SetOrthographicNearClip(orthoNear);
			}

			float orthoFar = camera.GetOrthographicFarClip();
			if (ImGui::DragFloat("Far", &orthoFar)) {
				camera.SetOrthographicFarClip(orthoFar);
			}

			ImGui::Checkbox("Fixed Aspect Ratio", &component->fixedAspect);
		}
	});

	DrawComponent<Component::PointLight>("Point Light", entity, [](auto& component) {
		ImGui::ColorEdit3("Color", glm::value_ptr(component->color));
		ImGui::DragFloat("Intensity", &component->lightIntensity, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Radius", &component->radius, 0.1f, 0.0f, 100.0f);
	});

	DrawComponent<Component::DirectionalLight>("Directional Light", entity, [](auto& component) {
		ImGui::ColorEdit3("Color", glm::value_ptr(component->color));
		ImGui::DragFloat("Intensity", &component->lightIntensity, 0.1f, 0.0f, 100.0f);
	});

	DrawComponent<Component::Model>("Model", entity, [](auto& component) {
		DrawVec3Control("Offset Position", component->offset.position, 0.0f, 110.0f);
		glm::vec3 rotation = glm::degrees(component->offset.rotation);
		DrawVec3Control("Offset Rotation", rotation, 0.0f, 110.0f);
		component->offset.rotation = glm::radians(rotation);
		DrawVec3Control("Offset Scale", component->offset.scale, 1.0f);
		ImGui::BeginDisabled(true);
		bool hasSkeleton = component->model->HasSkeleton();
		ImGui::Checkbox("Has skeleton", &hasSkeleton);
		ImGui::EndDisabled();
	});

	DrawComponent<Component::Animation>("Animation", entity, [](auto& component) {
		u32 animationIndex = 0;

		if (component->animationList) {
			std::string currentAnimationString = component->animationList->GetName(0);
			if (ImGui::BeginCombo("##Animation Clip", currentAnimationString.c_str())) {
				for (int i = 0; i < component->animationList->Size(); i++) {
					bool isSelected = currentAnimationString == component->animationList->GetName(i);
					if (ImGui::Selectable(component->animationList->GetName(i).c_str(), isSelected)) {
						currentAnimationString = component->animationList->GetName(i);
						u32 animationIndex     = i;
					}

					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			float lineHeight  = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImVec2 buttonSize = {lineHeight + 20.0f, lineHeight};
			ImGui::SameLine();
			if (ImGui::Button("Play", buttonSize)) {
				component->animationList->Start(animationIndex);
			}
		}
	});
}

template <typename T, typename UIFunction>
void Editor::DrawComponent(const std::string& name, Shared<Entity> entity, UIFunction uiFunction, bool removable) {
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
										   | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap
										   | ImGuiTreeNodeFlags_FramePadding;
	if (entity->HasComponent<T>()) {
		auto component                = entity->GetComponent<T>();
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImGui::Separator();
		bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
		ImGui::PopStyleVar();

		if (removable) {
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{lineHeight, lineHeight})) {
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings")) {
				if (ImGui::MenuItem("Remove component")) {
					removeComponent = true;
				}

				ImGui::EndPopup();
			}
			if (removeComponent) {
				entity->RemoveComponent<T>();
			}
		}

		if (open) {
			uiFunction(component);
			ImGui::TreePop();
		}
	}
}

void Editor::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth) {
	ImGuiIO& io   = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

	float lineHeight        = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize       = {lineHeight + 3.0f, lineHeight};
	ImDrawFlags buttonStyle = ImDrawFlags_RoundCornersLeft;
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
	ImGui::PushFont(boldFont);
	if (ImGui::Button("X", buttonSize)) {
		values.x = resetValue;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%g");
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
	ImGui::PushFont(boldFont);
	if (ImGui::Button("Y", buttonSize)) {
		values.y = resetValue;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%g");
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
	ImGui::PushFont(boldFont);
	if (ImGui::Button("Z", buttonSize)) {
		values.z = resetValue;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%g");
	ImGui::PopItemWidth();
	ImGui::PopStyleVar();
	ImGui::Columns(1);
	ImGui::PopID();
}

template <typename T>
void Editor::DisplayAddComponentEntry(const std::string& entryName) {
	if (!m_selectedEntity->HasComponent<T>()) {
		if (ImGui::MenuItem(entryName.c_str())) {
			m_selectedEntity->AddComponent<T>();
			ImGui::CloseCurrentPopup();
		}
	}
}

template <typename T>
void Editor::DisplayAddComponentFromFile(const std::string& entryName) {
	if (!m_selectedEntity->HasComponent<T>()) {
		if (ImGui::MenuItem(entryName.c_str())) {
			bool result = false;
			std::string filePath{""};
			result = OpenFileDialog(filePath);
			if (result && filePath != "") {
				m_selectedEntity->AddComponent<T>(filePath);
			}
			ImGui::CloseCurrentPopup();
		}
	}
}

void Editor::SetEditorStlye() {
	ImGuiStyle& style = ImGui::GetStyle();

	style.TabBarOverlineSize = 0.0f;
	style.TabBarBorderSize   = 0.0f;
	style.TabRounding        = 5.0f;
	style.WindowRounding     = 5.0f;
	style.FrameRounding      = 5.0f;
	style.GrabRounding       = 3.0f;
	style.DisabledAlpha      = 0.4f;
}

void Editor::SetEditorThemeColors() {
	auto& colors              = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

	// Headers
	colors[ImGuiCol_Header]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.9f, 0.3f, 0.0f, 0.8f);
	colors[ImGuiCol_HeaderActive]  = ImVec4{0.9f, 0.3f, 0.0f, 1.0f};

	// Buttons
	colors[ImGuiCol_Button]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
	colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
	colors[ImGuiCol_ButtonActive]  = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

	// Frame BG
	colors[ImGuiCol_FrameBg]        = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
	colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
	colors[ImGuiCol_FrameBgActive]  = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

	// Tabs
	colors[ImGuiCol_Tab]               = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
	colors[ImGuiCol_TabHovered]        = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
	colors[ImGuiCol_TabSelected]       = ImVec4{1.0f, 0.3529f, 0.0f, 1.0f};
	colors[ImGuiCol_TabDimmed]         = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
	colors[ImGuiCol_TabDimmedSelected] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

	// Title
	colors[ImGuiCol_TitleBg]          = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
	colors[ImGuiCol_TitleBgActive]    = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

	// Docking
	colors[ImGuiCol_DockingPreview] = ImVec4(0.9f, 0.3f, 0.0f, 0.8f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
}
}  // namespace Rava