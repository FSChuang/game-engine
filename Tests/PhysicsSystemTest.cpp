// Focused unit tests for Engine::PhysicsSystem (ENGINEERING_SPEC.md §10). Pure logic, no SDL
// window. Test values (gravity, deltaTime) are chosen as exact binary fractions so results
// are exactly representable in float, avoiding the need for epsilon-based comparison.

#include "Engine/GameObject/GameObject.h"
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

	// Act / Assert: SetGravity / GetGravity
	Engine::PhysicsSystem physics(10.0f);
	Check(physics.GetGravity() == 10.0f, "GetGravity_ReturnsConstructorValue");

	physics.SetGravity(20.0f);
	Check(physics.GetGravity() == 20.0f, "SetGravity_UpdatesStoredGravity");

	// Arrange: back to a simple gravity value for the motion checks below.
	physics.SetGravity(10.0f);

	// Act / Assert: gravity increases Y velocity by gravity * deltaTime. Update() itself
	// attaches the velocity property (via GameObject::SetVelocity) if the object did not
	// already have one.
	Engine::GameObject fallingObject(1, Engine::ObjectType::Generic, { 0.0f, 0.0f }, size);
	fallingObject.SetVelocity({ 0.0f, 0.0f });
	physics.Update(fallingObject, 0.5f);
	Check(fallingObject.GetVelocity().Y == 5.0f, "Update_AppliesGravityToYVelocity");

	// Assert: gravity must not touch X velocity.
	Engine::GameObject movingObject(2, Engine::ObjectType::Generic, { 0.0f, 0.0f }, size);
	movingObject.SetVelocity({ 3.0f, 0.0f });
	physics.Update(movingObject, 0.5f);
	Check(movingObject.GetVelocity().X == 3.0f, "Update_DoesNotChangeXVelocity");

	// Assert: position integrates using the already-updated velocity (semi-implicit Euler).
	Check(fallingObject.GetPosition().Y == 2.5f, "Update_IntegratesPositionUsingUpdatedVelocity");

	// Act / Assert: zero deltaTime must produce no change at all.
	Engine::GameObject idleObject(3, Engine::ObjectType::Generic, { 5.0f, 5.0f }, size);
	idleObject.SetVelocity({ 3.0f, 4.0f });
	physics.Update(idleObject, 0.0f);
	Check(idleObject.GetVelocity().X == 3.0f && idleObject.GetVelocity().Y == 4.0f,
	      "Update_WithZeroDeltaTime_DoesNotChangeVelocity");
	Check(idleObject.GetPosition().X == 5.0f && idleObject.GetPosition().Y == 5.0f,
	      "Update_WithZeroDeltaTime_DoesNotChangePosition");

	// Assert: a freshly-constructed object with no velocity property never had one invented
	// by the objects above — HasVelocity() is per-instance (PhysicsSystem::Update would
	// assert if called on an object like this; Milestone 3's "caller decides what gets
	// updated" contract, not exercised here since that path aborts the process).
	Engine::GameObject staticObject(4, Engine::ObjectType::Platform, { 0.0f, 0.0f }, size);
	Check(!staticObject.HasVelocity(), "GameObject_WithNoSetVelocityCall_HasNoVelocityProperty");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
