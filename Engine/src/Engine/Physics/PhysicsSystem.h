#pragma once

namespace Engine
{
	class Entity;

	// Applies a constant downward acceleration (gravity) to entities via semi-implicit Euler
	// integration. Holds no reference to any entity or entity list; the caller decides which
	// entities are affected by choosing whether to pass them to Update (ENGINEERING_SPEC.md §9:
	// gravity is configurable, never a value buried inside the system).
	class PhysicsSystem
	{
	public:
		explicit PhysicsSystem(float gravity);

		void SetGravity(float gravity);
		float GetGravity() const;

		// Adds gravity * deltaTime to the entity's Y velocity, then moves the entity by
		// velocity * deltaTime.
		void Update(Entity& entity, float deltaTime) const;

	private:
		float m_Gravity;
	};
}
