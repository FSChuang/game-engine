#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>

#include <array>

namespace Engine
{
	/// Thin wrapper over SDL's keyboard input. Held state (`IsKeyPressed`) is polled via
	/// `SDL_GetKeyboardState`. "Just pressed" is deliberately NOT derived from comparing two
	/// polled snapshots — a snapshot comparison can miss a genuine press that is released
	/// again before the next poll (most likely once a frame takes long enough that a quick
	/// tap's key-down AND key-up both land inside the same frame's event batch, e.g.
	/// several game windows competing for CPU). Observing SDL's own key-down event instead
	/// cannot miss it, however long the frame takes.
	class InputManager
	{
	public:
		/// Feeds one SDL event in. Call for every event `ProcessEvents()` pumps, each frame,
		/// before this frame's `IsKeyJustPressed()` calls. Only `SDL_EVENT_KEY_DOWN` events
		/// with `repeat == false` register as a "just pressed" edge — an already-held key's
		/// OS auto-repeat key-down events must never be mistaken for a new press.
		void ProcessEvent(const SDL_Event& event);

		/// Clears this frame's "just pressed" edges so they don't leak into the next frame.
		/// Call once per frame, after this frame's input has been read.
		void Update();

		/// True while the key is currently held down.
		bool IsKeyPressed(SDL_Scancode key) const;

		/// True only on the frame a genuine (non-repeat) key-down event for this key was
		/// observed via `ProcessEvent()`; false otherwise, including every frame the key is
		/// simply held afterward.
		bool IsKeyJustPressed(SDL_Scancode key) const;

	private:
		std::array<bool, SDL_SCANCODE_COUNT> m_JustPressedThisFrame{};
	};
}
