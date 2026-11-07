// Renderer sprite/texture API tests (Astro rebuild Checkpoint A).
//
// Runs against SDL's "dummy" video driver, so no real window or GPU is needed: it checks the
// parts of the API that are about ownership, validation and bookkeeping — NOT pixels. What
// actually appears on screen (rotation direction, alpha, sprite-sheet cropping) is checked by
// eye in the game, not here. The one piece of pure math, RadiansToDegrees, is tested directly.

#include "Engine/Renderer/Renderer.h"

#include <SDL3/SDL.h>

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

static int g_Failures = 0;

static void Check(bool condition, const char* name)
{
	if (condition)
	{
		std::printf("[PASS] %s\n", name);
	}
	else
	{
		std::printf("[FAIL] %s\n", name);
		++g_Failures;
	}
}

template<typename Exception, typename Callable>
static bool Throws(Callable callable)
{
	try
	{
		callable();
	}
	catch (const Exception&)
	{
		return true;
	}
	catch (...)
	{
		return false;
	}
	return false;
}

// Writes a tiny 2x2 RGBA PNG (one transparent pixel) and returns its path.
static std::string WriteTestPng()
{
	std::string path = std::string(SDL_GetBasePath()) + "renderer_test_sprite.png";

	SDL_Surface* surface = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_RGBA32);
	SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGBA(surface, 255, 0, 0, 255));
	SDL_Rect transparentPixel{ 1, 1, 1, 1 };
	SDL_FillSurfaceRect(surface, &transparentPixel, SDL_MapSurfaceRGBA(surface, 0, 0, 0, 0));
	bool saved = SDL_SavePNG(surface, path.c_str());
	SDL_DestroySurface(surface);

	if (!saved)
	{
		std::printf("Could not write test PNG: %s\n", SDL_GetError());
		std::exit(2);
	}
	return path;
}

int main()
{
	// --- pure math ---
	const float pi = 3.14159265358979323846f;
	Check(Engine::RadiansToDegrees(0.0f) == 0.0f, "RadiansToDegrees_Zero_IsZero");
	Check(std::fabs(Engine::RadiansToDegrees(pi) - 180.0f) < 1e-4f, "RadiansToDegrees_Pi_Is180");
	Check(std::fabs(Engine::RadiansToDegrees(pi / 2.0f) - 90.0f) < 1e-4f, "RadiansToDegrees_HalfPi_Is90");
	Check(std::fabs(Engine::RadiansToDegrees(-pi / 2.0f) + 90.0f) < 1e-4f, "RadiansToDegrees_Negative_KeepsSign");

	// --- Renderer against the dummy video driver ---
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::printf("SDL_Init failed: %s\n", SDL_GetError());
		return 2;
	}

	std::string png = WriteTestPng();

	{
		Engine::Renderer renderer(Engine::WindowConfig{ "RendererTest", 320, 180 });

		Engine::Color defaultClear = renderer.GetClearColor();
		Check(defaultClear.R == 0 && defaultClear.G == 0 && defaultClear.B == 255 && defaultClear.A == 255,
		      "ClearColor_Default_IsTheOriginalBlue");
		renderer.SetClearColor({ 10, 20, 30, 255 });
		Engine::Color changedClear = renderer.GetClearColor();
		Check(changedClear.R == 10 && changedClear.G == 20 && changedClear.B == 30, "SetClearColor_StoresNewColor");

		Engine::TextureId first = renderer.LoadTexture(png);
		Engine::TextureId second = renderer.LoadTexture(png);
		Check(first != Engine::InvalidTextureId && second != Engine::InvalidTextureId,
		      "LoadTexture_ReturnsValidIds");
		Check(first != second, "LoadTexture_SamePathTwice_GivesDistinctTextures");

		Check(Throws<std::runtime_error>([&] { renderer.LoadTexture(png + ".missing"); }),
		      "LoadTexture_MissingFile_Throws");
		Engine::TextureId third = renderer.LoadTexture(png);
		Check(third != first && third != second, "LoadTexture_AfterFailure_StillWorks");

		bool drawsFine = true;
		try
		{
			renderer.BeginFrame();
			renderer.DrawSprite(first, { 10.0f, 10.0f }, { 32.0f, 32.0f });
			renderer.DrawSprite(second, { 50.0f, 10.0f }, { 32.0f, 32.0f }, 1.0f);
			renderer.DrawSprite(third, { 90.0f, 10.0f }, { 32.0f, 32.0f }, 0.5f, Engine::SourceRect{ 0.0f, 0.0f, 1.0f, 1.0f });
			renderer.SetScalingMode(Engine::ScalingMode::Proportional);
			renderer.DrawSprite(first, { 10.0f, 50.0f }, { 32.0f, 32.0f });
			renderer.EndFrame();
		}
		catch (...)
		{
			drawsFine = false;
		}
		Check(drawsFine, "DrawSprite_ValidTextures_DoesNotThrow");

		Check(Throws<std::invalid_argument>([&] { renderer.DrawSprite(Engine::InvalidTextureId, { 0, 0 }, { 1, 1 }); }),
		      "DrawSprite_InvalidTextureId_Throws");
		Check(Throws<std::invalid_argument>([&] { renderer.DrawSprite(third + 1, { 0, 0 }, { 1, 1 }); }),
		      "DrawSprite_UnknownTextureId_Throws");
		// Renderer goes out of scope here with three textures loaded: ~Renderer must free them
		// before the SDL renderer (a wrong order is a crash or a sanitizer report).
	}

	SDL_Quit();
	std::remove(png.c_str());

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
