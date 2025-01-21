#include "ravapch.h"

// stb_image
#include <stb/stb_image.h>

#include "Framework/Window.h"

namespace Rava {
Window::Window(std::string_view name)
	: m_windowName(name) {
	InitWindow();
}

Window::~Window() {
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Window::InitWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	GLFWmonitor* primaryMonitor  = glfwGetPrimaryMonitor();
	const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

	int monitorX, monitorY;
	glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);

	m_window = glfwCreateWindow(m_width, m_height, m_windowName.c_str(), nullptr, nullptr);
	glfwSetWindowPos(m_window, monitorX + (videoMode->width - m_width) / 2, monitorY + (videoMode->height - m_height) / 2);

	glfwShowWindow(m_window);

	if (!glfwVulkanSupported()) {
		ENGINE_CRITICAL("GLFW: Vulkan not supported!");
	}

	// Set icon
	GLFWimage icon;
	int channels;
	std::string iconPathStr = "Assets/System/Rava.png";
	icon.pixels             = stbi_load(iconPathStr.c_str(), &icon.width, &icon.height, &channels, 4);
	if (icon.pixels) {
		glfwSetWindowIcon(m_window, 1, &icon);
		stbi_image_free(icon.pixels);
	}

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
}

void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto ravaWindow                  = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	ravaWindow->m_framebufferResized = true;
	ravaWindow->m_width              = width;
	ravaWindow->m_height             = height;
}
}  // namespace Rava
