#include "Engine/Input/InputManager.h"

#include <SDL3/SDL.h>

namespace Engine
{
	bool InputManager::IsKeyPressed(SDL_Scancode key) const
	{
		const bool* keyboardState = SDL_GetKeyboardState(nullptr);
		return keyboardState[key];
	}
}
