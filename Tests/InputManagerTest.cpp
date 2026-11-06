// Focused unit tests for Engine::InputManager's "just pressed" edge detection (bugfix
// checkpoint: GUI testing with four simultaneous client windows found jump input
// unreliable — root cause was polled-snapshot edge detection missing a key-down-then-up
// that completes within one slow frame; see InputManager.h's class comment). Pure logic:
// synthetic SDL_Event structs are constructed directly and fed to ProcessEvent() — no SDL
// subsystem needs to be initialized, since ProcessEvent()/IsKeyJustPressed() never touch
// live SDL state, only the event struct's own fields.

#include "Engine/Input/InputManager.h"

#include <cstdio>

static int g_Failures = 0;

static void Check(bool condition, const char* name)
{
	if (condition)
	{
		std::printf("[PASS] %s\n", name);
	}
	else
	{
		std::printf("[FAIL] %s\n", name);
		++g_Failures;
	}
}

static SDL_Event MakeKeyDown(SDL_Scancode scancode, bool repeat)
{
	SDL_Event event{};
	event.type = SDL_EVENT_KEY_DOWN;
	event.key.scancode = scancode;
	event.key.repeat = repeat;
	return event;
}

static SDL_Event MakeKeyUp(SDL_Scancode scancode)
{
	SDL_Event event{};
	event.type = SDL_EVENT_KEY_UP;
	event.key.scancode = scancode;
	event.key.repeat = false;
	return event;
}

int main()
{
	// A fresh (non-repeat) key-down produces JustPressed for exactly the frame it was
	// observed in, for each of the three jump keys independently.
	for (SDL_Scancode key : { SDL_SCANCODE_SPACE, SDL_SCANCODE_W, SDL_SCANCODE_UP })
	{
		Engine::InputManager input;
		Check(!input.IsKeyJustPressed(key), "FreshInputManager_HasNoJustPressedEdgeYet");

		input.ProcessEvent(MakeKeyDown(key, false));
		Check(input.IsKeyJustPressed(key), "NonRepeatKeyDown_ProducesJustPressedThisFrame");
	}

	// Critically: the key-down-then-up can both arrive before IsKeyJustPressed() is ever
	// read (exactly the "quick tap within one slow frame" scenario found during manual
	// GUI testing) and JustPressed must still be true — this is the actual bug fix.
	{
		Engine::InputManager input;
		input.ProcessEvent(MakeKeyDown(SDL_SCANCODE_SPACE, false));
		input.ProcessEvent(MakeKeyUp(SDL_SCANCODE_SPACE));
		Check(input.IsKeyJustPressed(SDL_SCANCODE_SPACE),
		      "KeyDownThenUpWithinOneFrame_StillProducesJustPressed_RegressionForManualGuiBug");
	}

	// JustPressed does not leak into the next frame once Update() clears it, even if the
	// physical key is still held (no new key-down event arrives while held).
	{
		Engine::InputManager input;
		input.ProcessEvent(MakeKeyDown(SDL_SCANCODE_SPACE, false));
		Check(input.IsKeyJustPressed(SDL_SCANCODE_SPACE), "BeforeUpdate_JustPressedIsTrue");

		input.Update();
		Check(!input.IsKeyJustPressed(SDL_SCANCODE_SPACE), "AfterUpdate_JustPressedEdgeIsCleared");
	}

	// Holding a key down: SDL delivers OS auto-repeat key-down events (repeat == true)
	// while held. Those must NEVER register as a new "just pressed" edge — holding jump
	// must not repeatedly auto-jump every frame.
	{
		Engine::InputManager input;
		input.ProcessEvent(MakeKeyDown(SDL_SCANCODE_SPACE, false));
		input.Update(); // simulates one frame boundary passing

		input.ProcessEvent(MakeKeyDown(SDL_SCANCODE_SPACE, true)); // OS auto-repeat while held
		Check(!input.IsKeyJustPressed(SDL_SCANCODE_SPACE), "HeldKey_AutoRepeatKeyDown_NeverProducesNewJustPressedEdge");
	}

	// Releasing and pressing again produces a genuinely new edge on the next real press.
	{
		Engine::InputManager input;
		input.ProcessEvent(MakeKeyDown(SDL_SCANCODE_SPACE, false));
		input.Update();
		input.ProcessEvent(MakeKeyUp(SDL_SCANCODE_SPACE));
		input.Update();
		Check(!input.IsKeyJustPressed(SDL_SCANCODE_SPACE), "AfterRelease_NoLingeringJustPressedEdge");

		input.ProcessEvent(MakeKeyDown(SDL_SCANCODE_SPACE, false));
		Check(input.IsKeyJustPressed(SDL_SCANCODE_SPACE), "PressAgainAfterRelease_ProducesANewEdge");
	}

	// Different keys' edges are independent of one another — pressing Space does not
	// mark W or Up as JustPressed too (the three jump keys must not interfere).
	{
		Engine::InputManager input;
		input.ProcessEvent(MakeKeyDown(SDL_SCANCODE_SPACE, false));
		Check(input.IsKeyJustPressed(SDL_SCANCODE_SPACE) && !input.IsKeyJustPressed(SDL_SCANCODE_W) &&
		          !input.IsKeyJustPressed(SDL_SCANCODE_UP),
		      "DistinctKeys_JustPressedEdgesAreIndependent");
	}

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
