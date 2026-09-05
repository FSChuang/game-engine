#include "Engine/Core/Application.h"
#include "Game.h"

#include <SDL3/SDL_main.h>

namespace
{
	// This game's window configuration (ENGINEERING_SPEC.md §9: tunables stay at the top level).
	constexpr int WindowWidth = 1920;
	constexpr int WindowHeight = 1080;
	const char* const WindowTitle = "Spare Parts";
}

int main(int argc, char* argv[])
{
	Engine::WindowConfig windowConfig{ WindowTitle, WindowWidth, WindowHeight };
	Engine::Application application(windowConfig);

	Game game;

	application.Run(
	    [&game](Engine::InputManager& input, float deltaTime) { game.Update(input, deltaTime); },
	    [&game](Engine::Renderer& renderer) { game.Render(renderer); });

	return 0;
}
