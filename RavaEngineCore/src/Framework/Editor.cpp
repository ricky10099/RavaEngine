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

	// pipeline cache is a potential future optimization, ignoring for now
	init_info.PipelineCache  = VK_NULL_HANDLE;
	init_info.DescriptorPool = m_descriptorPool;
	init_info.RenderPass     = renderPass;
	init_info.Subpass        = 0;

	// todo, I should probably get around to integrating a memory allocator library such as Vulkan
	// memory allocator (VMA) sooner than later. We don't want to have to update adding an allocator
	// in a ton of locations.
	init_info.Allocator       = VK_NULL_HANDLE;
	init_info.MinImageCount   = 2;
	init_info.ImageCount      = imageCount;
	init_info.CheckVkResultFn = CheckVKResult;
	ImGui_ImplVulkan_Init(&init_info);

	//// upload fonts, this is done by recording and submitting a one time use command buffer
	//// which can be done easily bye using some existing helper functions on the lve device object
	//auto commandBuffer = VK_Core::m_Device->BeginSingleTimeCommands(QueueTypes::GRAPHICS);
	//ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	//VK_Core::m_Device->EndSingleTimeCommands(commandBuffer, QueueTypes::GRAPHICS);
	//ImGui_ImplVulkan_DestroyFontUploadObjects();
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
}

void Editor::Run() {
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));  // Set transparent window background
	ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

	//ImGui::Begin("Vulkan Engine Debug Window");


	//auto callback = Imgui::m_Callback;
	//if (callback) {
	//	callback();
	//}

	//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
	ImGui::ShowDemoWindow();

	//ImGui::PopStyleColor();
}
}  // namespace Rava