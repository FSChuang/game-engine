#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Time/Timeline.h"

#include <functional>

namespace Engine
{
	/// Owns the SDL application lifecycle and the main game loop. The one deliberate engine
	/// singleton-like object (ENGINEERING_SPEC.md §9): a game is expected to create exactly one
	/// and call Run() on it. This is a documented intent, not an enforced invariant — nothing
	/// here guards against constructing a second instance.
	class Application
	{
	public:
		/// The per-frame update callback signature Application::Run accepts.
		using UpdateCallback = std::function<void(InputManager& input, float deltaTime)>;
		/// The per-frame render callback signature Application::Run accepts.
		using RenderCallback = std::function<void(Renderer& renderer)>;

		/// Constructs the window/renderer, input manager, and game timeline. Throws
		/// std::runtime_error if SDL_Init fails.
		explicit Application(const WindowConfig& windowConfig);
		/// Destroys the renderer before calling SDL_Quit().
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		/// Runs the main loop until quit is requested or the window is closed. Calls onUpdate
		/// once per frame with the input manager and the game timeline's elapsed seconds, then
		/// onRender once per frame with the renderer, between BeginFrame()/EndFrame().
		void Run(const UpdateCallback& onUpdate, const RenderCallback& onRender);

		/// The Timeline instance Run() samples every frame to produce deltaTime.
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
