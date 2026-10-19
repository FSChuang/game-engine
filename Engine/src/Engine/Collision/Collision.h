#pragma once

namespace Engine
{
	class GameObject;

	/// Axis-aligned bounding-box (AABB) overlap test using each object's position and size.
	/// Strict overlap: rectangles that only touch at an edge or corner do not count
	/// (ENGINEERING_SPEC.md §10). Works from Transform data alone, so a hidden object (no
	/// render property) collides exactly like a visible one — a death zone or side-scroll
	/// boundary needs no render/velocity property to participate in collision.
	bool IsColliding(const GameObject& a, const GameObject& b);
}
