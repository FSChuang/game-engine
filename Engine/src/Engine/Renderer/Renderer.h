#pragma once

#include "Engine/Core/Core.h"
#include "Engine/GameObject/GameObject.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace Engine
{
	/// Window/renderer configuration the game defines, not a value hidden inside the engine
	/// (ENGINEERING_SPEC.md §9: configurable data stays at the top level).
	struct WindowConfig
	{
		std::string Title;
		int Width;
		int Height;
	};

	/// How an entity's position/size are mapped onto the current window size.
	enum class ScalingMode
	{
		Constant,      ///< Rendered directly in pixel units; unaffected by window resizing.
		Proportional   ///< Scaled by currentWindowSize / referenceResolution.
	};

	/// Pure scaling math, independent of SDL, so it's unit-testable without opening a window
	/// (ENGINEERING_SPEC.md §10: scaling-mode conversion must be testable in isolation).
	/// Constant mode returns value unchanged; Proportional scales each axis independently by
	/// currentResolution / referenceResolution. Does not modify any Entity state itself.
	Vector2 ApplyScalingMode(Vector2 value, ScalingMode mode, Vector2 referenceResolution, Vector2 currentResolution);

	/// Returns the other scaling mode (Constant <-> Proportional). Pure logic used by
	/// Renderer::ToggleScalingMode, kept separate so it's testable without a window.
	ScalingMode NextScalingMode(ScalingMode mode);

	/// Opaque handle to a texture a Renderer owns. Only meaningful for the Renderer that
	/// returned it; the game never sees the underlying SDL_Texture.
	using TextureId = std::uint32_t;

	/// Never returned by LoadTexture, so a default-initialized id is always detectably unset.
	constexpr TextureId InvalidTextureId = 0;

	/// A region of a texture, in the texture's own pixels (not scaled with the window).
	/// Lets one texture hold several sprites, e.g. the frames of a sprite sheet.
	struct SourceRect
	{
		float X;
		float Y;
		float W;
		float H;
	};

	/// Radians -> degrees. The public sprite API takes radians; SDL's rotation takes degrees.
	/// Pure, so it is testable without a window.
	float RadiansToDegrees(float radians);

	/// Owns the SDL window and renderer for their entire lifetime (RAII, ENGINEERING_SPEC.md §6).
	/// Deliberately does not expose the underlying SDL_Window/SDL_Renderer to callers.
	class Renderer
	{
	public:
		/// Creates the SDL window and renderer together. Throws std::runtime_error if either
		/// creation call fails.
		explicit Renderer(const WindowConfig& config);
		/// Destroys every loaded texture, then the renderer, then the window.
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		/// Clears the frame to the current clear color (see SetClearColor), ready for drawing.
		void BeginFrame();

		/// Sets the color BeginFrame clears to. Blue {0, 0, 255, 255} until this is called.
		void SetClearColor(Color color);
		Color GetClearColor() const;

		/// Presents the completed frame to the window.
		void EndFrame();

		/// Draws a GameObject as a filled rectangle using its position, size, and color,
		/// scaled according to the current scaling mode; the color's alpha is honored (alpha
		/// blending). Objects with no render property
		/// (HasRender() == false) are silently skipped — a hidden object (spawn point, death
		/// zone, boundary) never needs to invent a color just to exist.
		void DrawObject(const GameObject& gameObject);

		/// Loads a PNG file into a texture the Renderer owns until it is destroyed, and
		/// returns its id. Pixel-art friendly: sampled nearest-neighbor, alpha blended.
		/// Loading the same path twice creates two textures. Throws std::runtime_error if
		/// the file cannot be read or the texture cannot be created.
		TextureId LoadTexture(const std::string& path);

		/// Draws (part of) a loaded texture. `position` and `size` mean exactly what they do
		/// for DrawObject (top-left corner and extent, in game units) and are scaled by the
		/// current scaling mode the same way. `rotationRadians` rotates the sprite clockwise
		/// on screen around the CENTER of the destination box. `source`, if given, selects
		/// the part of the texture to draw (texture pixels); otherwise the whole texture is
		/// drawn. Drawn in call order, over whatever was drawn before. Throws
		/// std::invalid_argument if `texture` was not returned by this Renderer's LoadTexture.
		void DrawSprite(TextureId texture, Vector2 position, Vector2 size, float rotationRadians = 0.0f,
		                std::optional<SourceRect> source = std::nullopt);

		/// Sets the scaling mode used by every subsequent DrawObject() and DrawSprite() call.
		void SetScalingMode(ScalingMode mode);
		/// The current scaling mode (Constant by default).
		ScalingMode GetScalingMode() const;
		/// Equivalent to SetScalingMode(NextScalingMode(GetScalingMode())).
		void ToggleScalingMode();

	private:
		Vector2 GetWindowSize() const;

		SDL_Window* m_Window;
		SDL_Renderer* m_Renderer;
		// Index = TextureId - 1. Every entry is owned here and destroyed in ~Renderer.
		std::vector<SDL_Texture*> m_Textures;
		Color m_ClearColor{ 0, 0, 255, 255 };
		Vector2 m_ReferenceResolution;
		ScalingMode m_ScalingMode = ScalingMode::Constant;
	};
}
