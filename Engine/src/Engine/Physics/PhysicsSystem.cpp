#include "Engine/Physics/PhysicsSystem.h"

#include "Engine/Entity/Entity.h"

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

	void PhysicsSystem::Update(Entity& entity, float deltaTime) const
	{
		Vector2 velocity = entity.GetVelocity();
		velocity.Y += m_Gravity * deltaTime;
		entity.SetVelocity(velocity);

		entity.Move({ velocity.X * deltaTime, velocity.Y * deltaTime });
	}
}
