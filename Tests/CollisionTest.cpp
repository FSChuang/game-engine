// Focused unit tests for Engine::IsColliding (ENGINEERING_SPEC.md §10). Pure logic, no SDL window.

#include "Engine/Collision/Collision.h"
#include "Engine/GameObject/GameObject.h"

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

static Engine::GameObject MakeObject(Engine::Vector2 position, Engine::Vector2 size)
{
	return Engine::GameObject(0, Engine::ObjectType::Generic, position, size);
}

int main()
{
	// Arrange / Act / Assert: overlapping rectangles.
	Engine::GameObject overlapA = MakeObject({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::GameObject overlapB = MakeObject({ 5.0f, 5.0f }, { 10.0f, 10.0f });
	Check(Engine::IsColliding(overlapA, overlapB), "IsColliding_OverlappingRectangles_ReturnsTrue");

	// Separated rectangles (gap on both axes).
	Engine::GameObject separatedA = MakeObject({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::GameObject separatedB = MakeObject({ 20.0f, 20.0f }, { 10.0f, 10.0f });
	Check(!Engine::IsColliding(separatedA, separatedB), "IsColliding_SeparatedRectangles_ReturnsFalse");

	// Touching horizontally at exactly one edge (same Y range, B starts where A ends on X).
	Engine::GameObject horizontalTouchA = MakeObject({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::GameObject horizontalTouchB = MakeObject({ 10.0f, 0.0f }, { 10.0f, 10.0f });
	Check(!Engine::IsColliding(horizontalTouchA, horizontalTouchB),
	      "IsColliding_TouchingHorizontalEdge_ReturnsFalse");

	// Touching vertically at exactly one edge (same X range, B starts where A ends on Y).
	Engine::GameObject verticalTouchA = MakeObject({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::GameObject verticalTouchB = MakeObject({ 0.0f, 10.0f }, { 10.0f, 10.0f });
	Check(!Engine::IsColliding(verticalTouchA, verticalTouchB), "IsColliding_TouchingVerticalEdge_ReturnsFalse");

	// Touching only at a single corner.
	Engine::GameObject cornerTouchA = MakeObject({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::GameObject cornerTouchB = MakeObject({ 10.0f, 10.0f }, { 10.0f, 10.0f });
	Check(!Engine::IsColliding(cornerTouchA, cornerTouchB), "IsColliding_TouchingCorner_ReturnsFalse");

	// One rectangle fully contains another.
	Engine::GameObject containerObject = MakeObject({ 0.0f, 0.0f }, { 20.0f, 20.0f });
	Engine::GameObject containedObject = MakeObject({ 5.0f, 5.0f }, { 5.0f, 5.0f });
	Check(Engine::IsColliding(containerObject, containedObject), "IsColliding_OneRectangleContainsAnother_ReturnsTrue");

	// Overlap using negative coordinates.
	Engine::GameObject negativeA = MakeObject({ -10.0f, -10.0f }, { 20.0f, 20.0f });
	Engine::GameObject negativeB = MakeObject({ -5.0f, -5.0f }, { 20.0f, 20.0f });
	Check(Engine::IsColliding(negativeA, negativeB), "IsColliding_OverlapWithNegativeCoordinates_ReturnsTrue");

	// Milestone 3: a hidden object (no render property, e.g. a death zone or spawn point)
	// collides exactly like a visible one — collision reads Transform data only.
	Engine::GameObject hiddenDeathZone(0, Engine::ObjectType::DeathZone, { 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::GameObject visiblePlayer = MakeObject({ 5.0f, 5.0f }, { 10.0f, 10.0f });
	Check(!hiddenDeathZone.HasRender(), "IsColliding_HiddenObject_HasNoRenderProperty");
	Check(Engine::IsColliding(hiddenDeathZone, visiblePlayer), "IsColliding_HiddenObjectVsVisibleObject_ReturnsTrue");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
