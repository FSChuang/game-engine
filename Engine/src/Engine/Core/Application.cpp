#include "Engine/Core/Application.h"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace Engine
{
	namespace
	{
		// Dev/debug key that toggles the renderer's scaling mode (Milestone 1 Task 6).
		constexpr SDL_Scancode ScalingModeToggleKey = SDL_SCANCODE_TAB;
	}

	Application::Application(const WindowConfig& windowConfig)
		: m_IsRunning(true)
	{
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			throw std::runtime_error(SDL_GetError());
		}

		try
		{
			m_Renderer = CreateScope<Renderer>(windowConfig);
		}
		catch (...)
		{
			// Renderer construction failed after SDL_Init succeeded: this constructor
			// will not complete, so ~Application will never run. Quit SDL here instead.
			SDL_Quit();
			throw;
		}
	}

	Application::~Application()
	{
		// Explicitly destroy the window/renderer before SDL_Quit(): member destruction
		// order alone must not be trusted to get this sequencing right.
		m_Renderer.reset();
		SDL_Quit();
	}

	void Application::Run(const UpdateCallback& onUpdate, const RenderCallback& onRender)
	{
		Uint64 previousTicks = SDL_GetTicks();

		while (m_IsRunning)
		{
			ProcessEvents();
			ProcessScalingModeToggle();

			Uint64 currentTicks = SDL_GetTicks();
			float deltaTime = static_cast<float>(currentTicks - previousTicks) / 1000.0f;
			previousTicks = currentTicks;

			onUpdate(m_Input, deltaTime);

			m_Renderer->BeginFrame();
			onRender(*m_Renderer);
			m_Renderer->EndFrame();

			m_Input.Update();
		}
	}

	void Application::ProcessScalingModeToggle()
	{
		if (m_Input.IsKeyJustPressed(ScalingModeToggleKey))
		{
			m_Renderer->ToggleScalingMode();
		}
	}

	void Application::ProcessEvents()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
			{
				m_IsRunning = false;
			}
		}
	}
}
