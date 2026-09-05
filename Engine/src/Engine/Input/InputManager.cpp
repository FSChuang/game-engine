#include "Engine/Input/InputManager.h"

#include <SDL3/SDL.h>

namespace Engine
{
	void InputManager::Update()
	{
		int keyCount = 0;
		const bool* keyboardState = SDL_GetKeyboardState(&keyCount);

		for (int i = 0; i < keyCount; ++i)
		{
			m_PreviousKeyState[i] = keyboardState[i];
		}
	}

	bool InputManager::IsKeyPressed(SDL_Scancode key) const
	{
		const bool* keyboardState = SDL_GetKeyboardState(nullptr);
		return keyboardState[key];
	}

	bool InputManager::IsKeyJustPressed(SDL_Scancode key) const
	{
		return IsKeyPressed(key) && !m_PreviousKeyState[key];
	}
}
