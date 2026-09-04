#include "Engine/Core/Application.h"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace Engine
{
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

	void Application::Run()
	{
		while (m_IsRunning)
		{
			ProcessEvents();

			m_Renderer->BeginFrame();
			m_Renderer->EndFrame();
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
