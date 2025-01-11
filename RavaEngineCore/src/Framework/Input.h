#pragma once

#include "Framework/InputEvents/KeyCode.h"
#include "Framework/InputEvents/MouseButton.h"


class Input {
   public:
	static bool IsKeyPress(KeyCode key);
	static bool IsKeyDown(KeyCode key);
	static bool IsKeyRepeat(KeyCode key, u32 frameCount = 2);
	static bool IsKeyReleas(KeyCode key);

	static bool IsMouseButtonPress(MouseButton button);
	static bool IsMouseButtonDown(MouseButton button);
	static bool IsMouseButtonRepeat(MouseButton button, u32 frameCount = 2);
	static bool IsMouseButtonRelease(MouseButton button);
	static glm::vec2 GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();

	private:
	static std::unordered_map<int, bool> m_keyProcessed;
	 static std::unordered_map<int, int> m_keyHoldFrames;
	static std::unordered_map<int, bool> m_mouseProcessed;
	static std::unordered_map<int, int> m_mouseHoldFrames;
};
