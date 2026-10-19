#pragma once

#include "Engine/Core/Core.h"

#include <cstdint>
#include <optional>

namespace Engine
{
	/// A flat RGBA color used to render a GameObject as a filled rectangle.
	struct Color
	{
		uint8_t R;
		uint8_t G;
		uint8_t B;
		uint8_t A;
	};

	/// Uniquely identifies one GameObject within whatever GameObjectRegistry created it.
	/// Not a network identity — see SetNetworkId/GetNetworkId for that.
	using ObjectId = std::uint32_t;

	/// Broad, game-agnostic categories a GameObject can belong to. Deliberately limited to
	/// concepts every 2D platform-style game needs (never a specific game's vocabulary —
	/// ENGINEERING_SPEC.md §0 & §5: the engine/game boundary is sacred). A game free to
	/// ignore this and treat every object as Generic; it exists only so collision/physics
	/// callers and a Scene can branch on "what kind of thing is this" without inheritance.
	enum class ObjectType : std::uint8_t
	{
		Generic = 0,  ///< Anything a game doesn't need to distinguish (e.g. a patrol enemy).
		Player,
		Platform,
		SpawnPoint,
		DeathZone,
		Boundary
	};

	/// A compositional 2D game object (Milestone 3): a stable identity and type, a required
	/// position/size, and three independently-optional properties — render color, velocity,
	/// and network identity. Composition instead of inheritance: a GameObject never becomes
	/// a Player/Platform/etc. subclass, it just has or lacks each property. A hidden object
	/// (spawn point, death zone) is simply one with no render property; a static platform is
	/// simply one with no velocity property. Deliberately still knows nothing about what a
	/// game actually does with it (ENGINEERING_SPEC.md §5 entity model note).
	///
	/// Rule of Zero: every member is a plain value (ObjectId, ObjectType, Vector2,
	/// std::optional<small POD>), so GameObject is freely copyable/movable with no custom
	/// special members and no heap allocation.
	class GameObject
	{
	public:
		GameObject(ObjectId id, ObjectType type, Vector2 position, Vector2 size);

		ObjectId GetId() const;
		ObjectType GetType() const;

		Vector2 GetPosition() const;
		/// Replaces the position outright.
		void SetPosition(Vector2 position);
		/// Adds delta to the current position; does not touch velocity.
		void Move(Vector2 delta);

		/// Fixed at construction — there is no setter.
		Vector2 GetSize() const;

		/// True once SetRender() has been called at least once.
		bool HasRender() const;
		/// Requires HasRender().
		Color GetColor() const;
		/// Attaches the render property if absent, or replaces its color if already present.
		void SetRender(Color color);

		/// True once SetVelocity() has been called at least once.
		bool HasVelocity() const;
		/// Requires HasVelocity().
		Vector2 GetVelocity() const;
		/// Attaches the velocity property if absent, or replaces its value if already present.
		void SetVelocity(Vector2 velocity);

		/// True once SetNetworkId() has been called at least once.
		bool HasNetworkId() const;
		/// Requires HasNetworkId(). Deliberately a plain std::uint32_t, not Engine::PlayerId —
		/// GameObject must not depend on the Network module (ENGINEERING_SPEC.md §0: the
		/// engine/game boundary, mirrored here as an engine-internal module boundary so the
		/// SDL-free object model never pulls in networking, and vice versa).
		std::uint32_t GetNetworkId() const;
		/// Attaches the network-identity property if absent, or replaces its value if already present.
		void SetNetworkId(std::uint32_t networkId);

	private:
		ObjectId m_Id;
		ObjectType m_Type;
		Vector2 m_Position;
		Vector2 m_Size;
		std::optional<Color> m_Render;
		std::optional<Vector2> m_Velocity;
		std::optional<std::uint32_t> m_NetworkId;
	};
}
