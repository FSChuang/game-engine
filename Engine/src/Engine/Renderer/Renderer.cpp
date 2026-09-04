#include "Engine/Renderer/Renderer.h"

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

	Renderer::Renderer(const WindowConfig& config)
		: m_Window(nullptr), m_Renderer(nullptr)
	{
		bool created = SDL_CreateWindowAndRenderer(config.Title.c_str(), config.Width, config.Height, 0, &m_Window,
		                                            &m_Renderer);
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
}
