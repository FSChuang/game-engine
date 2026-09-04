#pragma once

#include "Engine/Core/Core.h"

#include <cstdint>

namespace Engine
{
	// A flat RGBA color used to render an Entity as a filled rectangle.
	struct Color
	{
		uint8_t R;
		uint8_t G;
		uint8_t B;
		uint8_t A;
	};

	// A generic 2D game object: a position, a size, and the color used to draw it as a filled
	// rectangle. Deliberately knows nothing about what kind of object it represents (player,
	// enemy, wall, ...) so game-specific types can be layered on top later without this type
	// changing (ENGINEERING_SPEC.md §5 entity model note).
	class Entity
	{
	public:
		Entity(Vector2 position, Vector2 size, Color color);

		Vector2 GetPosition() const;
		void SetPosition(Vector2 position);
		void Move(Vector2 delta);

		Vector2 GetSize() const;
		Color GetColor() const;

		Vector2 GetVelocity() const;
		void SetVelocity(Vector2 velocity);

	private:
		Vector2 m_Position;
		Vector2 m_Size;
		Color m_Color;
		Vector2 m_Velocity{ 0.0f, 0.0f };
	};
}
