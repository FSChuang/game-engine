#include "Engine/Input/InputManager.h"

#include <SDL3/SDL.h>

namespace Engine
{
	void InputManager::ProcessEvent(const SDL_Event& event)
	{
		if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
		{
			m_JustPressedThisFrame[event.key.scancode] = true;
		}
	}

	void InputManager::Update()
	{
		m_JustPressedThisFrame.fill(false);
	}

	bool InputManager::IsKeyPressed(SDL_Scancode key) const
	{
		const bool* keyboardState = SDL_GetKeyboardState(nullptr);
		return keyboardState[key];
	}

	bool InputManager::IsKeyJustPressed(SDL_Scancode key) const
	{
		return m_JustPressedThisFrame[key];
	}
}
