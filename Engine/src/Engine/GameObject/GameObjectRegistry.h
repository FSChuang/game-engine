#pragma once

#include "Engine/GameObject/GameObject.h"

#include <cstdint>
#include <unordered_map>

namespace Engine
{
	/// Owns a set of GameObjects, keyed by the stable ObjectId it assigns each one at
	/// creation (Milestone 3). Deliberately just a thin wrapper over
	/// std::unordered_map<ObjectId, GameObject> — not a scene graph, not an ECS, no
	/// queries/systems registry. Any caller (a game's render/update loop, or a headless
	/// server's connection bookkeeping) can hold one of these and iterate it directly.
	///
	/// FindByNetworkId/RemoveByNetworkId exist because both ends of the network boundary
	/// need to resolve "the GameObject for remote/connected identity X" without maintaining
	/// their own separate id-translation table — see GameObject::SetNetworkId.
	class GameObjectRegistry
	{
	public:
		GameObjectRegistry() = default;

		/// Constructs a new GameObject with a fresh, never-reused ObjectId and returns a
		/// reference to it. The reference remains valid until this specific object is
		/// removed (standard unordered_map guarantee: inserting/erasing other elements
		/// never invalidates it).
		GameObject& Create(ObjectType type, Vector2 position, Vector2 size);

		/// Removes the object with the given id. Returns false (no-op) if no such object exists.
		bool Remove(ObjectId id);
		/// Removes the object whose network id equals `networkId`, if one exists. Returns
		/// false (no-op) if none is found.
		bool RemoveByNetworkId(std::uint32_t networkId);

		GameObject* Find(ObjectId id);
		const GameObject* Find(ObjectId id) const;

		/// Returns the first object whose HasNetworkId() is true and whose network id
		/// equals `networkId`, or nullptr if none is found. Linear scan — intended for the
		/// small (MaxPlayers-scale) object counts this engine targets, not large scenes.
		GameObject* FindByNetworkId(std::uint32_t networkId);
		const GameObject* FindByNetworkId(std::uint32_t networkId) const;

		std::size_t Count() const;

		// Plain iteration over every live object, so callers (Renderer loops, Physics
		// loops, a game's own update) can walk the whole registry without this class
		// inventing its own iterator type.
		std::unordered_map<ObjectId, GameObject>::iterator begin();
		std::unordered_map<ObjectId, GameObject>::iterator end();
		std::unordered_map<ObjectId, GameObject>::const_iterator begin() const;
		std::unordered_map<ObjectId, GameObject>::const_iterator end() const;

	private:
		std::unordered_map<ObjectId, GameObject> m_Objects;
		// Starts at 1 so 0 stays available as an "unassigned" sentinel elsewhere, mirroring
		// PlayerRegistry's own PlayerId convention.
		ObjectId m_NextId = 1;
	};
}
