#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Renderer/Renderer.h"

#include <functional>

namespace Engine
{
	// Owns the SDL application lifecycle and the main game loop.
	// The one deliberate engine singleton-like object (ENGINEERING_SPEC.md §9): a game
	// creates exactly one and calls Run() on it.
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
		// once per frame with the input manager and elapsed seconds, then onRender once per
		// frame with the renderer, between BeginFrame()/EndFrame().
		void Run(const UpdateCallback& onUpdate, const RenderCallback& onRender);

	private:
		void ProcessEvents();
		void ProcessScalingModeToggle();

		Scope<Renderer> m_Renderer;
		InputManager m_Input;
		bool m_IsRunning;
	};
}
