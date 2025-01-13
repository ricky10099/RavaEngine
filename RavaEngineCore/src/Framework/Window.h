#pragma once

#include "Framework/InputEvents/Event.h"

namespace Rava {
class Window {
   public:
	Window(std::string_view name);
	~Window();

	NO_COPY(Window)

	void ResetWindowResizedFlag() { m_framebufferResized = false; }

	u32 Width() const { return m_width; }
	u32 Height() const { return m_height; }
	bool ShouldClose() { return glfwWindowShouldClose(m_window); }
	VkExtent2D GetExtent() { return {static_cast<u32>(m_width), static_cast<u32>(m_height)}; }
	bool IsWindowResized() { return m_framebufferResized; }
	GLFWwindow* GetGLFWwindow() const { return m_window; }

   private:
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
	void InitWindow();

	/*static inline */u32 m_width               = 1280;
	/*static inline */u32 m_height              = 720;
	/*static inline */bool m_framebufferResized = false;

	std::string m_windowName;
	GLFWwindow* m_window;
};
}  // namespace Vulkan