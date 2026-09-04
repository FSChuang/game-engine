#include "Engine/Core/Application.h"

#include <SDL3/SDL_main.h>

namespace
{
	// This game's window configuration (ENGINEERING_SPEC.md §9: tunables stay at the top level).
	constexpr int WindowWidth = 1920;
	constexpr int WindowHeight = 1080;
	const char* const WindowTitle = "Game";
}

int main(int argc, char* argv[])
{
	Engine::WindowConfig windowConfig{ WindowTitle, WindowWidth, WindowHeight };
	Engine::Application application(windowConfig);
	application.Run();

	return 0;
}
