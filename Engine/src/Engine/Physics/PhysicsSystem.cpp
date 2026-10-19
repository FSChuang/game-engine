#include "Engine/Physics/PhysicsSystem.h"

#include "Engine/GameObject/GameObject.h"

namespace Engine
{
	PhysicsSystem::PhysicsSystem(float gravity)
		: m_Gravity(gravity)
	{
	}

	void PhysicsSystem::SetGravity(float gravity)
	{
		m_Gravity = gravity;
	}

	float PhysicsSystem::GetGravity() const
	{
		return m_Gravity;
	}

	void PhysicsSystem::Update(GameObject& object, float deltaTime) const
	{
		ENGINE_ASSERT(object.HasVelocity(), "PhysicsSystem::Update requires a velocity property");

		Vector2 velocity = object.GetVelocity();
		velocity.Y += m_Gravity * deltaTime;
		object.SetVelocity(velocity);

		object.Move({ velocity.X * deltaTime, velocity.Y * deltaTime });
	}
}
