#include "Engine/Collision/Collision.h"

#include "Engine/Entity/Entity.h"

namespace Engine
{
	bool IsColliding(const Entity& a, const Entity& b)
	{
		Vector2 aPosition = a.GetPosition();
		Vector2 aSize = a.GetSize();
		Vector2 bPosition = b.GetPosition();
		Vector2 bSize = b.GetSize();

		bool separatedOnX = aPosition.X + aSize.X <= bPosition.X || bPosition.X + bSize.X <= aPosition.X;
		bool separatedOnY = aPosition.Y + aSize.Y <= bPosition.Y || bPosition.Y + bSize.Y <= aPosition.Y;

		return !separatedOnX && !separatedOnY;
	}
}
