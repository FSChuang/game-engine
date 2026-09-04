#include "Engine/Entity/Entity.h"

namespace Engine
{
	Entity::Entity(Vector2 position, Vector2 size, Color color)
		: m_Position(position), m_Size(size), m_Color(color)
	{
	}

	Vector2 Entity::GetPosition() const
	{
		return m_Position;
	}

	void Entity::SetPosition(Vector2 position)
	{
		m_Position = position;
	}

	void Entity::Move(Vector2 delta)
	{
		m_Position.X += delta.X;
		m_Position.Y += delta.Y;
	}

	Vector2 Entity::GetSize() const
	{
		return m_Size;
	}

	Color Entity::GetColor() const
	{
		return m_Color;
	}
}
