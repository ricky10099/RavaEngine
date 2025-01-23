#include "ravapch.h"

#include "Framework/RavaEngine.h"
#include "Framework/Vulkan/Renderer.h"
#include "Framework/Resources/Texture.h"
#include "Framework/Resources/Skeleton.h"
#include "Framework/Components.h"

std::shared_ptr<Rava::Texture> g_TextureAtlas;
std::shared_ptr<Rava::Texture> g_TextureFontAtlas;
std::shared_ptr<Rava::Texture> g_DefaultTexture;
std::shared_ptr<Vulkan::Buffer> g_DummyBuffer;

namespace Vulkan {
std::unique_ptr<DescriptorPool> Renderer::s_descriptorPool;
Renderer::Renderer(Rava::Window* window)
	: m_ravaWindow{window}
	, m_frameCounter{0}
	, m_currentImageIndex{0}  //, m_ambientLightIntensity{0.0f}
	, m_currentFrameIndex{0}  //, m_showDebugShadowMap{false}
	, m_frameInProgress{false}
	, m_shadersCompiled{false} {
	// Init();
}

Renderer::~Renderer() {
	FreeCommandBuffers();
}

void Renderer::Init() {
	RecreateSwapChain();
	RecreateRenderpass();
	CreateCommandBuffers();

	for (u32 i = 0; i < m_uniformBuffers.size(); i++) {
		m_uniformBuffers[i] = std::make_unique<Buffer>(
			sizeof(GlobalUbo),
			1,  // u32 instanceCount
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VKContext->properties.limits.minUniformBufferOffsetAlignment
		);
		m_uniformBuffers[i]->Map();
	}

	// create a global pool for desciptor sets
	static constexpr u32 POOL_SIZE = 10000;

	s_descriptorPool = DescriptorPool::Builder()
						   .SetMaxSets(MAX_FRAMES_SYNC * POOL_SIZE)
						   .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_SYNC * 50)
						   .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_SYNC * 7500)
						   .AddPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, MAX_FRAMES_SYNC * 2450)
						   .Build();

	u32 dummy     = 0xffffffff;
	g_DummyBuffer = std::make_shared<Vulkan::Buffer>(sizeof(u32));
	g_DummyBuffer->Map();
	g_DummyBuffer->WriteToBuffer(&dummy);
	g_DummyBuffer->Flush();

	g_DefaultTexture = std::make_shared<Rava::Texture>(true);
	g_DefaultTexture->Init("Assets/System/Images/Rava.png", Rava::Texture::USE_SRGB);

	Unique<DescriptorSetLayout> globalDescriptorSetLayout =
		DescriptorSetLayout::Builder()
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)  // projection, view , lights
			//.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // spritesheet
			//.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // font atlas
			//.AddBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.Build();
	m_globalDescriptorSetLayout = globalDescriptorSetLayout->GetDescriptorSetLayout();

	Unique<DescriptorSetLayout> pbrMaterialDescriptorSetLayout =
		DescriptorSetLayout::Builder()
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)  // PBRUbo
			.AddBinding(
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT
			)  // diffuse color map
			.AddBinding(
				2,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT
			)  // normal map
			.AddBinding(
				3,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT
			)  // roughness metallic map
			.AddBinding(
				4,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT
			)  // emissive map
			.AddBinding(
				5,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT
			)  // roughness map
			.AddBinding(
				6,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT
			)  // metallic map
			.Build();

	Unique<DescriptorSetLayout> animationDescriptorSetLayout =
		DescriptorSetLayout::Builder()
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)  // shader data for animation
			.Build();

	std::vector<VkDescriptorSetLayout> descriptorSetLayoutsDefaultDiffuse = {m_globalDescriptorSetLayout};

	std::vector<VkDescriptorSetLayout> descriptorSetLayoutsPBR = {
		m_globalDescriptorSetLayout, pbrMaterialDescriptorSetLayout->GetDescriptorSetLayout()
	};
	std::vector<VkDescriptorSetLayout> descriptorSetLayoutsAnimation = {
		m_globalDescriptorSetLayout,
		pbrMaterialDescriptorSetLayout->GetDescriptorSetLayout(),
		animationDescriptorSetLayout->GetDescriptorSetLayout()
	};

	for (u32 i = 0; i < MAX_FRAMES_SYNC; i++) {
		VkDescriptorBufferInfo bufferInfo = m_uniformBuffers[i]->DescriptorInfo();
		DescriptorWriter(*globalDescriptorSetLayout, *s_descriptorPool)
			.WriteBuffer(0, &bufferInfo)
			//.WriteImage(1, imageInfo0)
			//.WriteImage(2, imageInfo1)
			.Build(m_globalDescriptorSets[i]);
	}

	m_entityRenderSystem = std::make_unique<EntityRenderSystem>(m_renderPass->Get3DRenderPass(), descriptorSetLayoutsAnimation);
	m_entityAnimationRenderSystem =
		std::make_unique<EntityAnimationRenderSystem>(m_renderPass->Get3DRenderPass(), descriptorSetLayoutsAnimation);
	m_pointLightRenderSystem =
		std::make_unique<PointLightRenderSystem>(m_renderPass->Get3DRenderPass(), m_globalDescriptorSetLayout);

	// m_Imgui = Imgui::Create(m_RenderPass->GetGUIRenderPass(), static_cast<u32>(m_SwapChain->ImageCount()));
	m_editor = std::make_unique<Rava::Editor>(m_renderPass->GetGUIRenderPass(), static_cast<u32>(m_swapChain->ImageCount()));
	for (u32 i = 0; i < m_swapChain->ImageCount(); ++i) {
		m_editor->RecreateDescriptorSet(m_swapChain->GetImageView(i), i);
	}
}

void Renderer::RecreateSwapChain() {
	auto extent = m_ravaWindow->GetExtent();
	while (extent.width == 0 || extent.height == 0) {
		extent = m_ravaWindow->GetExtent();
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(VKContext->GetLogicalDevice());

	// create the swapchain
	if (m_swapChain == nullptr) {
		m_swapChain = std::make_unique<SwapChain>(extent);
	} else {
		ENGINE_INFO("recreating swapchain at frame {0}", m_frameCounter);
		std::shared_ptr<SwapChain> oldSwapChain = std::move(m_swapChain);
		m_swapChain                             = std::make_unique<SwapChain>(extent, oldSwapChain);
		if (!oldSwapChain->CompareSwapFormats(*m_swapChain.get())) {
			ENGINE_CRITICAL("swap chain image or depth format has changed");
		}
	}

	if (m_editor) {
		for (u32 i = 0; i < m_swapChain->ImageCount(); ++i) {
			m_editor->RecreateDescriptorSet(m_swapChain->GetImageView(i), i);
		}
	}
}

void Renderer::RecreateRenderpass() {
	m_renderPass = std::make_shared<RenderPass>(m_swapChain.get());
}

void Renderer::CreateCommandBuffers() {
	// Resize command buffer count to have one for each framebuffer
	m_commandBuffers.resize(MAX_FRAMES_SYNC);
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // VK_COMMAND_BUFFER_LEVEL_PRIMARY : Buffer you submit firectly to
														   // queue. Can't be called by other buffers.
														   // VK_COMMAND_BUFFER_LEVEL_SECONDARY : Buffer can't ve called directly.
														   // Can be called from other buffers via "vkCmdExxcuteCommands" when
														   // recording commands in primary buffer.
	allocateInfo.commandPool        = VKContext->GetCommandPool();
	allocateInfo.commandBufferCount = static_cast<u32>(m_commandBuffers.size());

	// Allocate command buffers and place handles in array of buffers
	VkResult result = vkAllocateCommandBuffers(VKContext->GetLogicalDevice(), &allocateInfo, m_commandBuffers.data());
	VK_CHECK(result, "Failed to allocate Command Buffers!");
}

void Renderer::FreeCommandBuffers() {
	vkFreeCommandBuffers(
		VKContext->GetLogicalDevice(),
		VKContext->GetCommandPool(),
		static_cast<u32>(m_commandBuffers.size()),
		m_commandBuffers.data()
	);
	m_commandBuffers.clear();
}

void Renderer::BeginFrame() {
	ENGINE_ASSERT(!m_frameInProgress, "Can't Call BeginFrame while already in progress!");

	auto result = m_swapChain->AcquireNextImage(&m_currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		Recreate();
		// return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		ENGINE_CRITICAL("Failed to Acquire Swap Chain Image!");
	}

	m_frameInProgress = true;
	m_frameCounter++;

	auto commandBuffer = GetCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	VK_CHECK(result, "Failed to Begin Recording Command Buffer!")

	m_currentCommandBuffer = commandBuffer;
	// return commandBuffer;
	if (m_currentCommandBuffer) {
		m_frameInfo = {
			m_currentFrameIndex,
			0.0f, /* m_FrameTime */
			m_currentCommandBuffer,

			m_globalDescriptorSets[m_currentFrameIndex]
		};
	}

	m_editor->NewFrame();
}

void Renderer::UpdateAnimations(entt::registry& registry) {
	auto view = registry.view<Rava::Component::Model, Rava::Component::Transform, Rava::Component::Animation>();
	for (auto entity : view) {
		auto& mesh      = view.get<Rava::Component::Model>(entity);
		auto& animation = view.get<Rava::Component::Animation>(entity);
		auto skeleton   = mesh.model->GetSkeleton();
		if (mesh.enable) {
			animation.animationList->Update(*skeleton, m_frameCounter);
			mesh.model->UpdateAnimation(m_frameCounter);
		}
	}
}

void Renderer::RenderpassEntities(entt::registry& registry, Rava::Camera& currentCamera) {
	if (m_currentCommandBuffer) {
		GlobalUbo ubo{};
		ubo.projection  = currentCamera.GetProjection();
		ubo.view        = currentCamera.GetView();
		ubo.inverseView = currentCamera.GetInverseView();
		// ubo.Projection        = m_frameInfo.m_Camera->GetProjectionMatrix();
		// ubo.View              = m_frameInfo.m_Camera->GetViewMatrix();
		// ubo.AmbientLightColor = {1.0f, 1.0f, 1.0f, m_AmbientLightIntensity};
		// m_LightSystem->Update(m_FrameInfo, ubo, registry);
		// m_UniformBuffers[m_CurrentFrameIndex]->WriteToBuffer(&ubo);
		// m_UniformBuffers[m_CurrentFrameIndex]->Flush();

		// for (auto [entity, cam] : registry.view<Rava::Component::Camera>().each()) {
		//	if (cam.currentCamera) {
		//		ubo.projection  = cam.view.GetProjection();
		//		ubo.view        = cam.view.GetView();
		//		ubo.inverseView = cam.view.GetInverseView();
		//	}
		// }
		m_pointLightRenderSystem->Update(m_frameInfo, ubo, registry);
		m_uniformBuffers[m_currentFrameIndex]->WriteToBuffer(&ubo);
		m_uniformBuffers[m_currentFrameIndex]->Flush();

		Begin3DRenderPass(/*m_currentCommandBuffer*/);
		// BeginGUIRenderPass();
	}
}

void Renderer::RenderpassGUI() {
	if (m_currentCommandBuffer) {
		EndRenderPass();  // end 3D renderpass
		//m_swapChain->TransitionSwapChainImageLayout(
		//	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//	m_currentImageIndex,
		//	m_currentCommandBuffer
		//);
		BeginGUIRenderPass();
	}
}

void Renderer::EndFrame() {
	ENGINE_ASSERT(m_frameInProgress, "Can't Call EndFrame while Frame is not in progress!");
	auto commandBuffer = GetCurrentCommandBuffer();

	VkResult result = vkEndCommandBuffer(commandBuffer);
	VK_CHECK(result, "Failed to Record Command buffer!")

	result = m_swapChain->SubmitCommandBuffers(&commandBuffer, &m_currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_ravaWindow->IsWindowResized()) {
		m_ravaWindow->ResetWindowResizedFlag();
		Recreate();
	} else if (result != VK_SUCCESS) {
		ENGINE_CRITICAL("Failed to Present Swap Chain image!");
	}

	m_frameInProgress   = false;
	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_SYNC;
}

void Renderer::Begin3DRenderPass(/*VkCommandBuffer commandBuffer*/) {
	assert(m_frameInProgress);
	// assert(commandBuffer == GetCurrentCommandBuffer());

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass  = m_renderPass->Get3DRenderPass();
	renderPassInfo.framebuffer = m_renderPass->Get3DFrameBuffer(m_currentImageIndex);

	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = m_swapChain->GetSwapChainExtent();

	std::array<VkClearValue, static_cast<u32>(RenderPass::RenderTargets3D::NUMBER_OF_ATTACHMENTS)> clearValues{};
	clearValues[0].color = {
		Rava::Engine::s_Instance->clearColor.r,
		Rava::Engine::s_Instance->clearColor.g,
		Rava::Engine::s_Instance->clearColor.b,
		Rava::Engine::s_Instance->clearColor.a
	};
	clearValues[1].depthStencil    = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
	renderPassInfo.pClearValues    = clearValues.data();

	// vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBeginRenderPass(m_currentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = static_cast<float>(m_swapChain->GetSwapChainExtent().height);
	viewport.width    = static_cast<float>(m_swapChain->GetSwapChainExtent().width);
	viewport.height   = static_cast<float>(m_swapChain->GetSwapChainExtent().height) * -1.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{
		{0, 0},
        m_swapChain->GetSwapChainExtent()
	};
	vkCmdSetViewport(m_currentCommandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);
}

void Renderer::BeginGUIRenderPass(/*VkCommandBuffer commandBuffer*/) {
	assert(m_frameInProgress);
	// assert(commandBuffer == GetCurrentCommandBuffer());

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass  = m_renderPass->GetGUIRenderPass();
	renderPassInfo.framebuffer = m_renderPass->GetGUIFrameBuffer(m_currentImageIndex);

	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = m_swapChain->GetSwapChainExtent();

	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].color = {
		{0.01f, 0.01f, 0.01f, 1.0f}
	};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues    = clearValues.data();

	vkCmdBeginRenderPass(m_currentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = static_cast<float>(m_swapChain->GetSwapChainExtent().width);
	viewport.height   = static_cast<float>(m_swapChain->GetSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{
		{0, 0},
        m_swapChain->GetSwapChainExtent()
	};
	vkCmdSetViewport(m_currentCommandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);
}

void Renderer::EndRenderPass(/*VkCommandBuffer commandBuffer*/) const {
	assert(m_frameInProgress);
	// assert(commandBuffer == GetCurrentCommandBuffer());

	// vkCmdEndRenderPass(commandBuffer);
	vkCmdEndRenderPass(m_currentCommandBuffer);
}

void Renderer::UpdateEditor(Rava::Scene* scene) {
	m_editor->Organize(scene, m_currentImageIndex);
}

void Renderer::RenderEntities(Rava::Scene* scene) {
	if (m_currentCommandBuffer) {
		// UpdateTransformCache(scene, SceneGraph::ROOT_NODE, glm::mat4(1.0f), false);

		auto& registry = scene->GetRegistry();

		// 3D objects
		m_entityRenderSystem->Render(m_frameInfo, registry);
		m_entityAnimationRenderSystem->Render(m_frameInfo, registry);
		// m_RenderSystemPbrSA->RenderEntities(m_frameInfo, registry);
		// m_RenderSystemGrass->RenderEntities(m_frameInfo, registry);
	}
}

void Renderer::RenderEnv(entt::registry& registry) {
	if (m_currentCommandBuffer) {
		m_pointLightRenderSystem->Render(m_frameInfo, registry);
	}
}

void Renderer::EndScene() {
	if (m_currentCommandBuffer) {
		//m_swapChain->TransitionSwapChainImageLayout(
		//	VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//	m_currentImageIndex,
		//	m_currentCommandBuffer
		//);
		m_editor->Render(m_currentCommandBuffer);
		EndRenderPass(/*m_currentCommandBuffer*/);  // end GUI render pass
		/*m_swapChain->TransitionSwapChainImageLayout(
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, m_currentImageIndex, m_currentCommandBuffer
		);*/
		EndFrame();
		// m_swapChain->TransitionSwapChainImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	}
}

void Renderer::Recreate() {
	RecreateSwapChain();
	RecreateRenderpass();
	// CreateLightingDescriptorSets();
	// CreateRenderSystemBloom();
	// CreatePostProcessingDescriptorSets();
}

int Renderer::GetFrameIndex() const {
	assert(m_frameInProgress);
	return m_currentFrameIndex;
}

VkCommandBuffer Renderer::GetCurrentCommandBuffer() const {
	assert(m_frameInProgress);
	return m_commandBuffers[m_currentFrameIndex];
}
}  // namespace Vulkan