#include "Engine/Core/Application.h"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace Engine
{
	namespace
	{
		// Dev/debug key that toggles the renderer's scaling mode (Milestone 1 Task 6).
		constexpr SDL_Scancode ScalingModeToggleKey = SDL_SCANCODE_TAB;

		// Dev/debug keys demonstrating Milestone 2 §1 timeline control (ENGINEERING_SPEC.md §1:
		// named constants, not magic scancodes/literals).
		constexpr SDL_Scancode TimelinePauseToggleKey = SDL_SCANCODE_P;
		constexpr SDL_Scancode TimelineScaleHalfKey = SDL_SCANCODE_1;
		constexpr SDL_Scancode TimelineScaleNormalKey = SDL_SCANCODE_2;
		constexpr SDL_Scancode TimelineScaleDoubleKey = SDL_SCANCODE_3;

		constexpr double TimelineScaleHalf = 0.5;
		constexpr double TimelineScaleNormal = 1.0;
		constexpr double TimelineScaleDouble = 2.0;
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
			// Constructed here, after SDL_Init() has succeeded: Timeline's default constructor
			// reads a monotonic SDL time source, which must not run before SDL is initialized.
			m_GameTimeline = CreateScope<Timeline>();
		}
		catch (...)
		{
			// Renderer/Timeline construction failed after SDL_Init succeeded: this constructor
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
		while (m_IsRunning)
		{
			ProcessEvents();
			ProcessScalingModeToggle();
			ProcessTimelineControls();

			float deltaTime = static_cast<float>(m_GameTimeline->GetDeltaTime());

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

	void Application::ProcessTimelineControls()
	{
		if (m_Input.IsKeyJustPressed(TimelinePauseToggleKey))
		{
			if (m_GameTimeline->IsPaused())
			{
				m_GameTimeline->Unpause();
			}
			else
			{
				m_GameTimeline->Pause();
			}
		}

		// Scale changes are independent of pause state (ENGINEERING_SPEC.md §9: Command-Query
		// Separation — each key sets exactly one thing); Timeline::SetScale() never touches pause.
		if (m_Input.IsKeyJustPressed(TimelineScaleHalfKey))
		{
			m_GameTimeline->SetScale(TimelineScaleHalf);
		}
		else if (m_Input.IsKeyJustPressed(TimelineScaleNormalKey))
		{
			m_GameTimeline->SetScale(TimelineScaleNormal);
		}
		else if (m_Input.IsKeyJustPressed(TimelineScaleDoubleKey))
		{
			m_GameTimeline->SetScale(TimelineScaleDouble);
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

	Timeline& Application::GetGameTimeline()
	{
		return *m_GameTimeline;
	}
}
