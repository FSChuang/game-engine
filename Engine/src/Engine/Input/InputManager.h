#pragma once

#include <SDL3/SDL_scancode.h>

#include <array>

namespace Engine
{
	// Thin wrapper over SDL's polled keyboard state (ENGINEERING_SPEC.md §10: input abstraction).
	// Does not use SDL keyboard events.
	class InputManager
	{
	public:
		// Snapshots the current keyboard state for use by IsKeyJustPressed on the next call.
		// Call once per frame, after this frame's input has been read.
		void Update();

		// True while the key is currently held down.
		bool IsKeyPressed(SDL_Scancode key) const;

		// True only on the frame the key transitions from not-pressed to pressed; false while
		// the key continues to be held.
		bool IsKeyJustPressed(SDL_Scancode key) const;

	private:
		std::array<bool, SDL_SCANCODE_COUNT> m_PreviousKeyState{};
	};
}
