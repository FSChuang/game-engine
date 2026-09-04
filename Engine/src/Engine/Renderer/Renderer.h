#pragma once

#include <string>

struct SDL_Window;
struct SDL_Renderer;

namespace Engine
{
	// Window/renderer configuration the game defines, not a value hidden inside the engine
	// (ENGINEERING_SPEC.md §9: configurable data stays at the top level).
	struct WindowConfig
	{
		std::string Title;
		int Width;
		int Height;
	};

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

	private:
		SDL_Window* m_Window;
		SDL_Renderer* m_Renderer;
	};
}
