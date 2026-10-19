#include "Engine/GameObject/GameObject.h"

namespace Engine
{
	GameObject::GameObject(ObjectId id, ObjectType type, Vector2 position, Vector2 size)
		: m_Id(id), m_Type(type), m_Position(position), m_Size(size)
	{
	}

	ObjectId GameObject::GetId() const
	{
		return m_Id;
	}

	ObjectType GameObject::GetType() const
	{
		return m_Type;
	}

	Vector2 GameObject::GetPosition() const
	{
		return m_Position;
	}

	void GameObject::SetPosition(Vector2 position)
	{
		m_Position = position;
	}

	void GameObject::Move(Vector2 delta)
	{
		m_Position.X += delta.X;
		m_Position.Y += delta.Y;
	}

	Vector2 GameObject::GetSize() const
	{
		return m_Size;
	}

	bool GameObject::HasRender() const
	{
		return m_Render.has_value();
	}

	Color GameObject::GetColor() const
	{
		ENGINE_ASSERT(HasRender(), "GameObject::GetColor requires SetRender() to have been called");
		return *m_Render;
	}

	void GameObject::SetRender(Color color)
	{
		m_Render = color;
	}

	bool GameObject::HasVelocity() const
	{
		return m_Velocity.has_value();
	}

	Vector2 GameObject::GetVelocity() const
	{
		ENGINE_ASSERT(HasVelocity(), "GameObject::GetVelocity requires SetVelocity() to have been called");
		return *m_Velocity;
	}

	void GameObject::SetVelocity(Vector2 velocity)
	{
		m_Velocity = velocity;
	}

	bool GameObject::HasNetworkId() const
	{
		return m_NetworkId.has_value();
	}

	std::uint32_t GameObject::GetNetworkId() const
	{
		ENGINE_ASSERT(HasNetworkId(), "GameObject::GetNetworkId requires SetNetworkId() to have been called");
		return *m_NetworkId;
	}

	void GameObject::SetNetworkId(std::uint32_t networkId)
	{
		m_NetworkId = networkId;
	}
}
