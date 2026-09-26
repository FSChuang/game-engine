#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Time/Timeline.h"

#include <functional>

namespace Engine
{
	// Owns the SDL application lifecycle and the main game loop.
	// The one deliberate engine singleton-like object (ENGINEERING_SPEC.md §9): a game is
	// expected to create exactly one and call Run() on it. This is a documented intent, not
	// an enforced invariant — nothing here guards against constructing a second instance.
	class Application
	{
	public:
		// The callback seam through which game code participates in the loop, without any
		// gameplay logic living in Engine/ (ENGINEERING_SPEC.md §0, §5).
		using UpdateCallback = std::function<void(InputManager& input, float deltaTime)>;
		using RenderCallback = std::function<void(Renderer& renderer)>;

		explicit Application(const WindowConfig& windowConfig);
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		// Runs the main loop until quit is requested or the window is closed. Calls onUpdate
		// once per frame with the input manager and the game timeline's elapsed seconds, then
		// onRender once per frame with the renderer, between BeginFrame()/EndFrame().
		void Run(const UpdateCallback& onUpdate, const RenderCallback& onRender);

		// The engine's "game time" timeline (Milestone 2 §1). Its elapsed time drives onUpdate's
		// deltaTime. Exposed so game code can pause/unpause or change scale in response to its
		// own input handling; Application itself already demonstrates this via debug keys, the
		// same way it already owns the renderer-scaling debug toggle.
		Timeline& GetGameTimeline();

	private:
		void ProcessEvents();
		void ProcessScalingModeToggle();
		void ProcessTimelineControls();

		Scope<Renderer> m_Renderer;
		InputManager m_Input;
		bool m_IsRunning;
		// Constructed in the constructor body, after SDL_Init() succeeds: its default anchor
		// reads a monotonic SDL time source, which must not run before SDL is initialized.
		Scope<Timeline> m_GameTimeline;
	};
}
