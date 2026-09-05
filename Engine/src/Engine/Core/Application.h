#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Renderer/Renderer.h"

namespace Engine
{
	// Owns the SDL application lifecycle and the main game loop.
	// The one deliberate engine singleton-like object (ENGINEERING_SPEC.md §9): a game
	// creates exactly one and calls Run() on it.
	class Application
	{
	public:
		explicit Application(const WindowConfig& windowConfig);
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		// Runs the main loop until quit is requested or the window is closed.
		void Run();

	private:
		void ProcessEvents();
		void ProcessScalingModeToggle();

		Scope<Renderer> m_Renderer;
		InputManager m_Input;
		bool m_IsRunning;
	};
}
