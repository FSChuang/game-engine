#include "Engine/Renderer/Renderer.h"

#include "Engine/Entity/Entity.h"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace Engine
{
	namespace
	{
		// Milestone 1 Task 1 clears every frame to solid blue (ENGINEERING_SPEC.md §1: no magic numbers).
		constexpr Uint8 ClearColorRed = 0;
		constexpr Uint8 ClearColorGreen = 0;
		constexpr Uint8 ClearColorBlue = 255;
		constexpr Uint8 ClearColorAlpha = 255;
	}

	Vector2 ApplyScalingMode(Vector2 value, ScalingMode mode, Vector2 referenceResolution, Vector2 currentResolution)
	{
		if (mode == ScalingMode::Constant)
		{
			return value;
		}

		float scaleX = currentResolution.X / referenceResolution.X;
		float scaleY = currentResolution.Y / referenceResolution.Y;

		return { value.X * scaleX, value.Y * scaleY };
	}

	ScalingMode NextScalingMode(ScalingMode mode)
	{
		return mode == ScalingMode::Constant ? ScalingMode::Proportional : ScalingMode::Constant;
	}

	Renderer::Renderer(const WindowConfig& config)
		: m_Window(nullptr), m_Renderer(nullptr),
		  m_ReferenceResolution{ static_cast<float>(config.Width), static_cast<float>(config.Height) }
	{
		bool created = SDL_CreateWindowAndRenderer(config.Title.c_str(), config.Width, config.Height,
		                                            SDL_WINDOW_RESIZABLE, &m_Window, &m_Renderer);
		if (created)
		{
			return;
		}

		std::string error = SDL_GetError();

		if (m_Renderer != nullptr)
		{
			SDL_DestroyRenderer(m_Renderer);
			m_Renderer = nullptr;
		}

		if (m_Window != nullptr)
		{
			SDL_DestroyWindow(m_Window);
			m_Window = nullptr;
		}

		throw std::runtime_error(error);
	}

	Renderer::~Renderer()
	{
		SDL_DestroyRenderer(m_Renderer);
		SDL_DestroyWindow(m_Window);
	}

	void Renderer::BeginFrame()
	{
		SDL_SetRenderDrawColor(m_Renderer, ClearColorRed, ClearColorGreen, ClearColorBlue, ClearColorAlpha);
		SDL_RenderClear(m_Renderer);
	}

	void Renderer::EndFrame()
	{
		SDL_RenderPresent(m_Renderer);
	}

	void Renderer::DrawEntity(const Entity& entity)
	{
		Vector2 currentResolution = GetWindowSize();
		Vector2 position = ApplyScalingMode(entity.GetPosition(), m_ScalingMode, m_ReferenceResolution, currentResolution);
		Vector2 size = ApplyScalingMode(entity.GetSize(), m_ScalingMode, m_ReferenceResolution, currentResolution);
		Color color = entity.GetColor();

		SDL_SetRenderDrawColor(m_Renderer, color.R, color.G, color.B, color.A);

		SDL_FRect rect{ position.X, position.Y, size.X, size.Y };
		SDL_RenderFillRect(m_Renderer, &rect);
	}

	void Renderer::SetScalingMode(ScalingMode mode)
	{
		m_ScalingMode = mode;
	}

	ScalingMode Renderer::GetScalingMode() const
	{
		return m_ScalingMode;
	}

	void Renderer::ToggleScalingMode()
	{
		m_ScalingMode = NextScalingMode(m_ScalingMode);
	}

	Vector2 Renderer::GetWindowSize() const
	{
		int width = 0;
		int height = 0;
		SDL_GetWindowSize(m_Window, &width, &height);
		return { static_cast<float>(width), static_cast<float>(height) };
	}
}
