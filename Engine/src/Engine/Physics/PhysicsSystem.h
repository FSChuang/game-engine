#pragma once

namespace Engine
{
	class GameObject;

	/// Applies a constant downward acceleration (gravity) to objects via semi-implicit Euler
	/// integration. Holds no reference to any object or object list; the caller decides
	/// which objects are affected by choosing whether to pass them to Update
	/// (ENGINEERING_SPEC.md §9: gravity is configurable, never a value buried inside the
	/// system). Only objects with a velocity property can be updated — a static platform or
	/// a hidden spawn point/death zone is simply never passed here.
	class PhysicsSystem
	{
	public:
		explicit PhysicsSystem(float gravity);

		void SetGravity(float gravity);
		float GetGravity() const;

		/// Adds gravity * deltaTime to the object's Y velocity, then moves the object by
		/// velocity * deltaTime. Requires object.HasVelocity() — the caller decides what
		/// gets updated by choosing what it passes here.
		/// @param object Object to update; must have a velocity property.
		/// @param deltaTime Elapsed time in seconds.
		void Update(GameObject& object, float deltaTime) const;

	private:
		float m_Gravity;
	};
}
