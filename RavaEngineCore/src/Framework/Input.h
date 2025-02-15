#pragma once

#include "Framework/InputEvents/KeyCode.h"
#include "Framework/InputEvents/MouseButton.h"

class Input {
   public:
	static bool IsKeyPress(KeyCode key);
	static bool IsKeyDown(KeyCode key);
	static bool IsKeyRepeat(KeyCode key, u32 frameCount = 2);
	static bool IsKeyReleas(KeyCode key);

	static bool IsMouseButtonPress(MouseButton button = Mouse::Button0);
	static bool IsMouseButtonDown(MouseButton button = Mouse::Button0);
	static bool IsMouseButtonRepeat(MouseButton button = Mouse::Button0, u32 frameCount = 2);
	static bool IsMouseButtonRelease(MouseButton button = Mouse::Button0);
	static glm::vec2 GetMousePosition();
	static float GetMouseX();
	static float GetMouseY();

   private:
	static std::unordered_map<int, bool> m_keyProcessed;
	static std::unordered_map<int, u32> m_keyHoldFrames;
	static std::unordered_map<int, bool> m_mouseProcessed;
	static std::unordered_map<int, u32> m_mouseHoldFrames;
};
