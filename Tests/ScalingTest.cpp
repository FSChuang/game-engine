// Focused unit tests for Engine::ApplyScalingMode / Engine::NextScalingMode (ENGINEERING_SPEC.md
// §10). Pure logic, no SDL window. Test values are chosen as exact binary fractions so results
// are exactly representable in float, avoiding the need for epsilon-based comparison.

#include "Engine/Renderer/Renderer.h"

#include <cstdio>

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

int main()
{
	// Arrange
	Engine::Vector2 referenceResolution{ 1920.0f, 1080.0f };
	Engine::Vector2 value{ 100.0f, 200.0f };

	// Act / Assert: Constant mode leaves the value unchanged, even when the window is resized.
	Engine::Vector2 resizedResolution{ 960.0f, 540.0f };
	Engine::Vector2 constantResult =
	    Engine::ApplyScalingMode(value, Engine::ScalingMode::Constant, referenceResolution, resizedResolution);
	Check(constantResult.X == 100.0f && constantResult.Y == 200.0f, "ApplyScalingMode_Constant_LeavesValueUnchanged");

	// Act / Assert: Proportional scaling from 1920x1080 to 960x540 is 0.5 on both axes.
	Engine::Vector2 halvedResult =
	    Engine::ApplyScalingMode(value, Engine::ScalingMode::Proportional, referenceResolution, resizedResolution);
	Check(halvedResult.X == 50.0f && halvedResult.Y == 100.0f,
	      "ApplyScalingMode_ProportionalHalfResolution_ScalesByHalfOnBothAxes");

	// Act / Assert: a non-uniform resize scales X and Y independently.
	Engine::Vector2 squareReference{ 1000.0f, 1000.0f };
	Engine::Vector2 nonUniformResolution{ 500.0f, 2000.0f };
	Engine::Vector2 nonUniformValue{ 10.0f, 10.0f };
	Engine::Vector2 nonUniformResult =
	    Engine::ApplyScalingMode(nonUniformValue, Engine::ScalingMode::Proportional, squareReference, nonUniformResolution);
	Check(nonUniformResult.X == 5.0f && nonUniformResult.Y == 20.0f,
	      "ApplyScalingMode_NonUniformResize_ScalesXAndYIndependently");

	// Act / Assert: toggling flips Constant <-> Proportional in both directions.
	Check(Engine::NextScalingMode(Engine::ScalingMode::Constant) == Engine::ScalingMode::Proportional,
	      "NextScalingMode_FromConstant_ReturnsProportional");
	Check(Engine::NextScalingMode(Engine::ScalingMode::Proportional) == Engine::ScalingMode::Constant,
	      "NextScalingMode_FromProportional_ReturnsConstant");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
