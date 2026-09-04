// Focused unit tests for Engine::PhysicsSystem (ENGINEERING_SPEC.md §10). Pure logic, no SDL window.
// Test values (gravity, deltaTime) are chosen as exact binary fractions so results are exactly
// representable in float, avoiding the need for epsilon-based comparison.

#include "Engine/Entity/Entity.h"
#include "Engine/Physics/PhysicsSystem.h"

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
	Engine::Vector2 size{ 10.0f, 10.0f };
	Engine::Color color{ 255, 255, 255, 255 };

	// Act / Assert: SetGravity / GetGravity
	Engine::PhysicsSystem physics(10.0f);
	Check(physics.GetGravity() == 10.0f, "GetGravity_ReturnsConstructorValue");

	physics.SetGravity(20.0f);
	Check(physics.GetGravity() == 20.0f, "SetGravity_UpdatesStoredGravity");

	// Arrange: back to a simple gravity value for the motion checks below.
	physics.SetGravity(10.0f);

	// Act / Assert: gravity increases Y velocity by gravity * deltaTime.
	Engine::Entity fallingEntity({ 0.0f, 0.0f }, size, color);
	physics.Update(fallingEntity, 0.5f);
	Check(fallingEntity.GetVelocity().Y == 5.0f, "Update_AppliesGravityToYVelocity");

	// Assert: gravity must not touch X velocity.
	Engine::Entity movingEntity({ 0.0f, 0.0f }, size, color);
	movingEntity.SetVelocity({ 3.0f, 0.0f });
	physics.Update(movingEntity, 0.5f);
	Check(movingEntity.GetVelocity().X == 3.0f, "Update_DoesNotChangeXVelocity");

	// Assert: position integrates using the already-updated velocity (semi-implicit Euler).
	Check(fallingEntity.GetPosition().Y == 2.5f, "Update_IntegratesPositionUsingUpdatedVelocity");

	// Act / Assert: zero deltaTime must produce no change at all.
	Engine::Entity idleEntity({ 5.0f, 5.0f }, size, color);
	idleEntity.SetVelocity({ 3.0f, 4.0f });
	physics.Update(idleEntity, 0.0f);
	Check(idleEntity.GetVelocity().X == 3.0f && idleEntity.GetVelocity().Y == 4.0f,
	      "Update_WithZeroDeltaTime_DoesNotChangeVelocity");
	Check(idleEntity.GetPosition().X == 5.0f && idleEntity.GetPosition().Y == 5.0f,
	      "Update_WithZeroDeltaTime_DoesNotChangePosition");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
