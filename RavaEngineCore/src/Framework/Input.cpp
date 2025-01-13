#include "ravapch.h"

#include "Framework/RavaEngine.h"
#include "Framework/Input.h"

std::unordered_map<int, bool> Input::m_keyProcessed;
std::unordered_map<int, int> Input::m_keyHoldFrames;
std::unordered_map<int, bool> Input::m_mouseProcessed;
std::unordered_map<int, int> Input::m_mouseHoldFrames;

bool Input::IsKeyPress(KeyCode key) {
	auto* window = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	auto state   = glfwGetKey(window, static_cast<int>(key));
	return state == GLFW_PRESS;
}

bool Input::IsKeyDown(KeyCode key) {
	auto* window = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	auto keyCode = static_cast<int>(key);
	auto state   = glfwGetKey(window, keyCode);

	if (state == GLFW_PRESS && !m_keyProcessed[keyCode]) {
		m_keyProcessed[keyCode] = true;
		return true;
	}

	if (state == GLFW_RELEASE) {
		m_keyProcessed[keyCode] = false;
	}

	return false;
}

bool Input::IsKeyRepeat(KeyCode key, u32 frameCount) {
	auto* window = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	auto keyInt  = static_cast<int>(key);
	auto state   = glfwGetKey(window, keyInt);

	if (state == GLFW_PRESS) {
		m_keyHoldFrames[keyInt]++;
		if (m_keyHoldFrames[keyInt] >= frameCount) {
			return true;
		}
	} else {
		m_keyHoldFrames[keyInt] = 0;
	}

	return false;
}

bool Input::IsKeyReleas(KeyCode key) {
	auto* window = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	auto state   = glfwGetKey(window, static_cast<int>(key));
	return state == GLFW_RELEASE;
}

bool Input::IsMouseButtonPress(MouseButton button) {
	auto* window = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	auto state   = glfwGetMouseButton(window, static_cast<int>(button));
	return state == GLFW_PRESS;
}

bool Input::IsMouseButtonDown(MouseButton button) {
	auto* window     = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	auto mouseButton = static_cast<int>(button);
	auto state       = glfwGetMouseButton(window, mouseButton);

	if (state == GLFW_PRESS && !m_mouseProcessed[mouseButton]) {
		m_mouseProcessed[mouseButton] = true;
		return true;
	}

	if (state == GLFW_RELEASE) {
		m_mouseProcessed[mouseButton] = false;
	}

	return false;
}

bool Input::IsMouseButtonRepeat(MouseButton button, u32 frameCount) {
	auto* window     = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	auto mouseButton = static_cast<int>(button);
	auto state       = glfwGetMouseButton(window, mouseButton);

	if (state == GLFW_PRESS) {
		m_mouseHoldFrames[mouseButton]++;
		if (m_mouseHoldFrames[mouseButton] >= frameCount) {
			return true;
		}
	} else {
		m_mouseHoldFrames[mouseButton] = 0;
	}

	return false;
}

bool Input::IsMouseButtonRelease(MouseButton button) {
	auto* window = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	auto state   = glfwGetMouseButton(window, static_cast<int>(button));
	return state == GLFW_RELEASE;
}

glm::vec2 Input::GetMousePosition() {
	auto* window = static_cast<GLFWwindow*>(Rava::Engine::s_Instance->GetGLFWWindow());
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	return {(float)xpos, (float)ypos};
}

float Input::GetMouseX() {
	return GetMousePosition().x;
}

float Input::GetMouseY() {
	return GetMousePosition().y;
}
