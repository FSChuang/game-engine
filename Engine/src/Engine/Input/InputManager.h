#pragma once

#include <SDL3/SDL_scancode.h>

namespace Engine
{
	// Thin wrapper over SDL's polled keyboard state (ENGINEERING_SPEC.md §10: input abstraction).
	// Answers whether a key is currently held down; does not use SDL keyboard events.
	class InputManager
	{
	public:
		bool IsKeyPressed(SDL_Scancode key) const;
	};
}
