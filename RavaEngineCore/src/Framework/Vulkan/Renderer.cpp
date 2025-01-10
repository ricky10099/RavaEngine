#include "ravapch.h"

#include "Framework/Vulkan/Renderer.h"

namespace Vulkan {
std::unique_ptr<DescriptorPool> Renderer::s_descriptorPool;
Renderer::Renderer(Rava::Window* window)
	: m_ravaWindow{window}
	, m_frameCounter{0}
	, m_currentImageIndex{0}  //, m_ambientLightIntensity{0.0f}
	, m_currentFrameIndex{0}  //, m_showDebugShadowMap{false}
	, m_frameInProgress{false}
	, m_shadersCompiled{false} {
	ENGINE_INFO("Initializing Renderer.");
	// Init();
	//  CompileShaders();  // runs in a parallel thread and sets m_ShadersCompiled
}

Renderer::~Renderer() {
	ENGINE_INFO("Destruct Renderer");
	// vkDeviceWaitIdle(VKContext->GetLogicalDevice());
	FreeCommandBuffers();
}

void Renderer::Init() {
	// if (!m_shadersCompiled) {
	//	return m_shadersCompiled;
	// }

	RecreateSwapChain();
	RecreateRenderpass();
	CreateCommandBuffers();

	for (u32 i = 0; i < m_uniformBuffers.size(); i++) {
		m_uniformBuffers[i] = std::make_unique<Buffer>(
			sizeof(GlobalUbo),
			1,  // u32 instanceCount
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VKContext->Properties.limits.minUniformBufferOffsetAlignment
		);
		m_uniformBuffers[i]->Map();
	}

	// create a global pool for desciptor sets
	static constexpr u32 POOL_SIZE = 10000;
	s_descriptorPool               = DescriptorPool::Builder()
						   .SetMaxSets(MAX_FRAMES_SYNC * POOL_SIZE)
						   .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_SYNC * 50)
						   .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_SYNC * 7500)
						   .AddPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, MAX_FRAMES_SYNC * 2450)
						   .Build();

	std::unique_ptr<DescriptorSetLayout> globalDescriptorSetLayout =
		DescriptorSetLayout::Builder()
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)          // projection, view , lights
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // spritesheet
			.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // font atlas
			.AddBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.Build();
	m_globalDescriptorSetLayout = globalDescriptorSetLayout->GetDescriptorSetLayout();

	std::vector<VkDescriptorSetLayout> descriptorSetLayoutsDefaultDiffuse = {m_globalDescriptorSetLayout};

	for (u32 i = 0; i < MAX_FRAMES_SYNC; i++) {
		VkDescriptorBufferInfo bufferInfo = m_uniformBuffers[i]->DescriptorInfo();
		DescriptorWriter(*globalDescriptorSetLayout, *s_descriptorPool)
			.WriteBuffer(0, &bufferInfo)
			//.WriteImage(1, imageInfo0)
			//.WriteImage(2, imageInfo1)
			.Build(m_globalDescriptorSets[i]);
	}

	// m_Imgui = Imgui::Create(m_RenderPass->GetGUIRenderPass(), static_cast<u32>(m_SwapChain->ImageCount()));
	m_editor = std::make_unique<Rava::Editor>(m_renderPass->Get3DRenderPass(), static_cast<u32>(m_swapChain->ImageCount()));
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

	auto commandBuffer = GetCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	VK_CHECK(result, "Failed to Begin Recording Command Buffer!")

	m_currentCommandBuffer = commandBuffer;
	// return commandBuffer;

	m_editor->NewFrame();
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
		{0.1f, 0.01f, 0.01f, 1.0f}
	};
	clearValues[1].depthStencil    = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<u32>(clearValues.size());
	renderPassInfo.pClearValues    = clearValues.data();

	// vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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

// void Renderer::BeginGUIRenderPass(/*VkCommandBuffer commandBuffer*/) {
//	assert(m_frameInProgress);
//	//assert(commandBuffer == GetCurrentCommandBuffer());
//
//	VkRenderPassBeginInfo renderPassInfo{};
//	renderPassInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//	renderPassInfo.renderPass  = m_renderPass->GetGUIRenderPass();
//	renderPassInfo.framebuffer = m_renderPass->GetGUIFrameBuffer(m_currentImageIndex);
//
//	renderPassInfo.renderArea.offset = {0, 0};
//	renderPassInfo.renderArea.extent = m_swapChain->GetSwapChainExtent();
//
//	std::array<VkClearValue, 1> clearValues{};
//	clearValues[0].color = {
//		{0.01f, 0.01f, 0.01f, 1.0f}
//	};
//	renderPassInfo.clearValueCount   = 1;
//	renderPassInfo.pClearValues    = clearValues.data();
//
//	vkCmdBeginRenderPass(m_currentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//
//	VkViewport viewport{};
//	viewport.x        = 0.0f;
//	viewport.y        = 0.0f;
//	viewport.width    = static_cast<float>(m_swapChain->GetSwapChainExtent().width);
//	viewport.height   = static_cast<float>(m_swapChain->GetSwapChainExtent().height);
//	viewport.minDepth = 0.0f;
//	viewport.maxDepth = 1.0f;
//	VkRect2D scissor{
//		{0, 0},
//         m_swapChain->GetSwapChainExtent()
//	};
//	vkCmdSetViewport(m_currentCommandBuffer, 0, 1, &viewport);
//	vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);
// }

void Renderer::EndRenderPass(/*VkCommandBuffer commandBuffer*/) {
	assert(m_frameInProgress);
	// assert(commandBuffer == GetCurrentCommandBuffer());

	// vkCmdEndRenderPass(commandBuffer);
	vkCmdEndRenderPass(m_currentCommandBuffer);
}

void Renderer::UpdateEditor() {
	m_editor->Run();
}

void Renderer::EndScene() {
	if (m_currentCommandBuffer) {
		m_editor->Render(m_currentCommandBuffer);
		EndRenderPass(/*m_currentCommandBuffer*/);  // end GUI render pass
		EndFrame();
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