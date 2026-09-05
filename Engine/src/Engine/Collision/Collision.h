#pragma once

namespace Engine
{
	class Entity;

	// Axis-aligned bounding-box (AABB) overlap test using each entity's position and size.
	// Strict overlap: rectangles that only touch at an edge or corner do not count
	// (ENGINEERING_SPEC.md §10).
	bool IsColliding(const Entity& a, const Entity& b);
}
