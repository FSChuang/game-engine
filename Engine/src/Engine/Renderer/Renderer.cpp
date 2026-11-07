#include "Engine/Renderer/Renderer.h"

#include "Engine/GameObject/GameObject.h"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace Engine
{
	namespace
	{
		constexpr float DegreesPerRadian = 180.0f / 3.14159265358979323846f;
	}

	float RadiansToDegrees(float radians)
	{
		return radians * DegreesPerRadian;
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
			// So DrawObject honors Color alpha. (RenderClear is unaffected by the blend mode.)
			SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_BLEND);
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
		// Textures belong to the SDL renderer: they must be destroyed first.
		for (SDL_Texture* texture : m_Textures)
		{
			SDL_DestroyTexture(texture);
		}

		SDL_DestroyRenderer(m_Renderer);
		SDL_DestroyWindow(m_Window);
	}

	void Renderer::BeginFrame()
	{
		SDL_SetRenderDrawColor(m_Renderer, m_ClearColor.R, m_ClearColor.G, m_ClearColor.B, m_ClearColor.A);
		SDL_RenderClear(m_Renderer);
	}

	void Renderer::SetClearColor(Color color)
	{
		m_ClearColor = color;
	}

	Color Renderer::GetClearColor() const
	{
		return m_ClearColor;
	}

	void Renderer::EndFrame()
	{
		SDL_RenderPresent(m_Renderer);
	}

	void Renderer::DrawObject(const GameObject& gameObject)
	{
		if (!gameObject.HasRender())
		{
			return;
		}

		Vector2 currentResolution = GetWindowSize();
		Vector2 position =
		    ApplyScalingMode(gameObject.GetPosition(), m_ScalingMode, m_ReferenceResolution, currentResolution);
		Vector2 size = ApplyScalingMode(gameObject.GetSize(), m_ScalingMode, m_ReferenceResolution, currentResolution);
		Color color = gameObject.GetColor();

		SDL_SetRenderDrawColor(m_Renderer, color.R, color.G, color.B, color.A);

		SDL_FRect rect{ position.X, position.Y, size.X, size.Y };
		SDL_RenderFillRect(m_Renderer, &rect);
	}

	TextureId Renderer::LoadTexture(const std::string& path)
	{
		SDL_Surface* surface = SDL_LoadPNG(path.c_str());
		if (surface == nullptr)
		{
			throw std::runtime_error("LoadTexture: cannot load '" + path + "': " + SDL_GetError());
		}

		SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
		// The texture holds its own copy of the pixels; the surface is not needed either way.
		SDL_DestroySurface(surface);
		if (texture == nullptr)
		{
			throw std::runtime_error("LoadTexture: cannot create texture for '" + path + "': " + SDL_GetError());
		}

		bool configured = SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) &&
		                  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		if (!configured)
		{
			std::string error = SDL_GetError();
			SDL_DestroyTexture(texture);
			throw std::runtime_error("LoadTexture: cannot configure texture for '" + path + "': " + error);
		}

		try
		{
			m_Textures.push_back(texture);
		}
		catch (...)
		{
			SDL_DestroyTexture(texture);
			throw;
		}

		return static_cast<TextureId>(m_Textures.size()); // ids start at 1; 0 is InvalidTextureId
	}

	void Renderer::DrawSprite(TextureId texture, Vector2 position, Vector2 size, float rotationRadians,
	                          std::optional<SourceRect> source)
	{
		if (texture == InvalidTextureId || texture > m_Textures.size())
		{
			throw std::invalid_argument("DrawSprite: unknown TextureId " + std::to_string(texture));
		}

		Vector2 currentResolution = GetWindowSize();
		Vector2 scaledPosition = ApplyScalingMode(position, m_ScalingMode, m_ReferenceResolution, currentResolution);
		Vector2 scaledSize = ApplyScalingMode(size, m_ScalingMode, m_ReferenceResolution, currentResolution);

		SDL_FRect destination{ scaledPosition.X, scaledPosition.Y, scaledSize.X, scaledSize.Y };

		SDL_FRect sourceRect{};
		const SDL_FRect* sourcePointer = nullptr;
		if (source.has_value())
		{
			sourceRect = SDL_FRect{ source->X, source->Y, source->W, source->H };
			sourcePointer = &sourceRect;
		}

		// A null center means "rotate around the middle of the destination rectangle".
		SDL_RenderTextureRotated(m_Renderer, m_Textures[texture - 1], sourcePointer, &destination,
		                         RadiansToDegrees(rotationRadians), nullptr, SDL_FLIP_NONE);
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
