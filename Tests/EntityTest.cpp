// Focused unit tests for Engine::Entity (ENGINEERING_SPEC.md §10). Pure logic, no SDL window.

#include "Engine/Entity/Entity.h"

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
	Engine::Vector2 initialPosition{ 10.0f, 20.0f };
	Engine::Vector2 size{ 32.0f, 64.0f };
	Engine::Color color{ 255, 0, 0, 255 };
	Engine::Entity entity(initialPosition, size, color);

	// Assert (one logical assertion per test)
	Check(entity.GetPosition().X == 10.0f && entity.GetPosition().Y == 20.0f,
	      "Entity_InitialPosition_MatchesConstructorArgument");

	// Act
	entity.SetPosition({ 5.0f, 7.0f });

	// Assert
	Check(entity.GetPosition().X == 5.0f && entity.GetPosition().Y == 7.0f, "SetPosition_UpdatesPosition");

	// Act
	entity.Move({ 3.0f, -2.0f });

	// Assert
	Check(entity.GetPosition().X == 8.0f && entity.GetPosition().Y == 5.0f, "Move_AddsDeltaToCurrentPosition");

	// Assert
	Check(entity.GetSize().X == 32.0f && entity.GetSize().Y == 64.0f, "Move_DoesNotChangeSize");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
