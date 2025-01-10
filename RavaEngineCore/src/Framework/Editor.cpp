#include "ravapch.h"

#include "Framework/Editor.h"
#include "Framework/Vulkan/VKUtils.h"
#include "Framework/RavaEngine.h"

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

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	// Initialize imgui for vulkan
	ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)Engine::s_engine->GetGLFWWindow(), true);
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

	//// upload fonts, this is done by recording and submitting a one time use command buffer
	//// which can be done easily bye using some existing helper functions on the lve device object
	// auto commandBuffer = VK_Core::m_Device->BeginSingleTimeCommands(QueueTypes::GRAPHICS);
	// ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	// VK_Core::m_Device->EndSingleTimeCommands(commandBuffer, QueueTypes::GRAPHICS);
	// ImGui_ImplVulkan_DestroyFontUploadObjects();
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
	ENGINE_INFO("Destruct Editor");
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
}

void Editor::Render(VkCommandBuffer commandBuffer) {
	ImGui::Render();
	ImDrawData* drawdata = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawdata, commandBuffer);

	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void Editor::Run() {
	// static bool dockspaceOpen                 = true;
	// static bool opt_fullscreen_persistant     = true;
	// bool opt_fullscreen                       = opt_fullscreen_persistant;
	// static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
	// ImGuiWindowFlags window_flags             = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	// if (opt_fullscreen) {
	//	ImGuiViewport* viewport = ImGui::GetMainViewport();
	//	ImGui::SetNextWindowPos(viewport->Pos);
	//	ImGui::SetNextWindowSize(viewport->Size);
	//	ImGui::SetNextWindowViewport(viewport->ID);
	//	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	//	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	//	window_flags |=
	//		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	//	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	// }

	// if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
	//	window_flags |= ImGuiWindowFlags_NoBackground;
	// }

	// ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	// ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	// ImGui::PopStyleVar();

	// if (opt_fullscreen) {
	//	ImGui::PopStyleVar(2);
	// }

	//// DockSpace
	// ImGuiIO& io           = ImGui::GetIO();
	// ImGuiStyle& style     = ImGui::GetStyle();
	// float minWinSizeX     = style.WindowMinSize.x;
	// style.WindowMinSize.x = 370.0f;
	// if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
	//	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	//	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	// }

	// style.WindowMinSize.x = minWinSizeX;

	// if (ImGui::BeginMenuBar()) {
	//	if (ImGui::BeginMenu("File")) {
	//		if (ImGui::MenuItem("Open Project...", "Ctrl+O")) {
	//			// OpenProject();
	//		}

	//		ImGui::Separator();

	//		if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
	//			// NewScene();
	//		}

	//		if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
	//			// SaveScene();
	//		}

	//		if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
	//			// SaveSceneAs();
	//		}

	//		ImGui::Separator();

	//		if (ImGui::MenuItem("Exit")) {
	//			// Application::Get().Close();
	//		}

	//		ImGui::EndMenu();
	//	}

	//	if (ImGui::BeginMenu("Script")) {
	//		if (ImGui::MenuItem("Reload assembly", "Ctrl+R")) {
	//			// ScriptEngine::ReloadAssembly();
	//		}

	//		ImGui::EndMenu();
	//	}

	//	ImGui::EndMenuBar();
	//}

	// ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));  // Set transparent window background
	// ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

	// ImGui::Begin("Vulkan Engine Debug Window");

	// auto callback = Imgui::m_Callback;
	// if (callback) {
	//	callback();
	// }

	// ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	// ImGui::End();

	// ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
	//ImGui::Begin("Viewport");
	//auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	//auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	//auto viewportOffset    = ImGui::GetWindowPos();
	//m_viewportBounds[0]    = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
	//m_viewportBounds[1]    = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

	//m_viewportFocused = ImGui::IsWindowFocused();
	//m_viewportHovered = ImGui::IsWindowHovered();

	//// Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);
	//// m_descriptorSets = ImGui_ImplVulkan_AddTexture(m_textureSampler, Engine::s_engine->Get)
	//ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	//m_viewportSize           = {viewportPanelSize.x, viewportPanelSize.y};
	//// ImGui_ImplVulkan_AddTexture() uint64_t textureID = m_framebuffer->GetColorAttachmentRendererID();
	//ImGui::Image(
	//	(ImTextureID)m_descriptorSets[Engine::s_engine->GetCurrentFrameIndex()], ImVec2{m_viewportSize.x, m_viewportSize.y}
	//);

	//ImGui::End();
	// ImGui::PopStyleVar();

	ImGui::ShowDemoWindow();

	// ImGui::PopStyleColor();
}

void Editor::RecreateDescriptorSet(VkImageView swapChainImage, u32 currentFrame) {
	m_descriptorSets[currentFrame] =
		ImGui_ImplVulkan_AddTexture(m_textureSampler, swapChainImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
}  // namespace Rava