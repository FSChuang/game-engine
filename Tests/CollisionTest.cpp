// Focused unit tests for Engine::IsColliding (ENGINEERING_SPEC.md §10). Pure logic, no SDL window.

#include "Engine/Collision/Collision.h"
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

static Engine::Entity MakeEntity(Engine::Vector2 position, Engine::Vector2 size)
{
	Engine::Color color{ 255, 255, 255, 255 };
	return Engine::Entity(position, size, color);
}

int main()
{
	// Arrange / Act / Assert: overlapping rectangles.
	Engine::Entity overlapA = MakeEntity({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::Entity overlapB = MakeEntity({ 5.0f, 5.0f }, { 10.0f, 10.0f });
	Check(Engine::IsColliding(overlapA, overlapB), "IsColliding_OverlappingRectangles_ReturnsTrue");

	// Separated rectangles (gap on both axes).
	Engine::Entity separatedA = MakeEntity({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::Entity separatedB = MakeEntity({ 20.0f, 20.0f }, { 10.0f, 10.0f });
	Check(!Engine::IsColliding(separatedA, separatedB), "IsColliding_SeparatedRectangles_ReturnsFalse");

	// Touching horizontally at exactly one edge (same Y range, B starts where A ends on X).
	Engine::Entity horizontalTouchA = MakeEntity({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::Entity horizontalTouchB = MakeEntity({ 10.0f, 0.0f }, { 10.0f, 10.0f });
	Check(!Engine::IsColliding(horizontalTouchA, horizontalTouchB),
	      "IsColliding_TouchingHorizontalEdge_ReturnsFalse");

	// Touching vertically at exactly one edge (same X range, B starts where A ends on Y).
	Engine::Entity verticalTouchA = MakeEntity({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::Entity verticalTouchB = MakeEntity({ 0.0f, 10.0f }, { 10.0f, 10.0f });
	Check(!Engine::IsColliding(verticalTouchA, verticalTouchB), "IsColliding_TouchingVerticalEdge_ReturnsFalse");

	// Touching only at a single corner.
	Engine::Entity cornerTouchA = MakeEntity({ 0.0f, 0.0f }, { 10.0f, 10.0f });
	Engine::Entity cornerTouchB = MakeEntity({ 10.0f, 10.0f }, { 10.0f, 10.0f });
	Check(!Engine::IsColliding(cornerTouchA, cornerTouchB), "IsColliding_TouchingCorner_ReturnsFalse");

	// One rectangle fully contains another.
	Engine::Entity containerEntity = MakeEntity({ 0.0f, 0.0f }, { 20.0f, 20.0f });
	Engine::Entity containedEntity = MakeEntity({ 5.0f, 5.0f }, { 5.0f, 5.0f });
	Check(Engine::IsColliding(containerEntity, containedEntity), "IsColliding_OneRectangleContainsAnother_ReturnsTrue");

	// Overlap using negative coordinates.
	Engine::Entity negativeA = MakeEntity({ -10.0f, -10.0f }, { 20.0f, 20.0f });
	Engine::Entity negativeB = MakeEntity({ -5.0f, -5.0f }, { 20.0f, 20.0f });
	Check(Engine::IsColliding(negativeA, negativeB), "IsColliding_OverlapWithNegativeCoordinates_ReturnsTrue");

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
