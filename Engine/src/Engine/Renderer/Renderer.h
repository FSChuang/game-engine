#pragma once

#include "Engine/Core/Core.h"

#include <string>

struct SDL_Window;
struct SDL_Renderer;

namespace Engine
{
	class Entity;

	// Window/renderer configuration the game defines, not a value hidden inside the engine
	// (ENGINEERING_SPEC.md §9: configurable data stays at the top level).
	struct WindowConfig
	{
		std::string Title;
		int Width;
		int Height;
	};

	// How an entity's position/size are mapped onto the current window size.
	enum class ScalingMode
	{
		Constant,      // Rendered directly in pixel units; unaffected by window resizing.
		Proportional   // Scaled by currentWindowSize / referenceResolution.
	};

	// Pure scaling math, independent of SDL, so it's unit-testable without opening a window
	// (ENGINEERING_SPEC.md §10: scaling-mode conversion must be testable in isolation).
	// Constant mode returns value unchanged; Proportional scales each axis independently by
	// currentResolution / referenceResolution. Does not modify any Entity state itself.
	Vector2 ApplyScalingMode(Vector2 value, ScalingMode mode, Vector2 referenceResolution, Vector2 currentResolution);

	// Returns the other scaling mode (Constant <-> Proportional). Pure logic used by
	// Renderer::ToggleScalingMode, kept separate so it's testable without a window.
	ScalingMode NextScalingMode(ScalingMode mode);

	// Owns the SDL window and renderer for their entire lifetime (RAII, ENGINEERING_SPEC.md §6).
	// Deliberately does not expose the underlying SDL_Window/SDL_Renderer to callers.
	class Renderer
	{
	public:
		explicit Renderer(const WindowConfig& config);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		// Clears the frame to the engine's background color, ready for drawing.
		void BeginFrame();

		// Presents the completed frame to the window.
		void EndFrame();

		// Draws an entity as a filled rectangle using its position, size, and color, scaled
		// according to the current scaling mode.
		void DrawEntity(const Entity& entity);

		void SetScalingMode(ScalingMode mode);
		ScalingMode GetScalingMode() const;
		void ToggleScalingMode();

	private:
		Vector2 GetWindowSize() const;

		SDL_Window* m_Window;
		SDL_Renderer* m_Renderer;
		Vector2 m_ReferenceResolution;
		ScalingMode m_ScalingMode = ScalingMode::Constant;
	};
}
