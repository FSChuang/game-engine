#pragma once

#include "Engine/Network/Protocol.h"

#include <optional>
#include <unordered_map>
#include <vector>

namespace Engine
{
	/// Pure server-side bookkeeping for which players are currently active and their
	/// latest reported state. No sockets, no SDL, no threads, and no internal
	/// synchronization — not thread-safe if shared across server threads.
	class PlayerRegistry
	{
	public:
		PlayerRegistry() = default;

		/// Assigns a new player a unique ID (never reused, even after removal) and a
		/// default, all-zero initial state. Returns std::nullopt if MaxPlayers are
		/// already active.
		std::optional<PlayerId> AssignPlayer();

		/// Overwrites the stored state for `state.Id`. Returns false (no-op) if
		/// `state.Id` is not a currently active player.
		bool UpdatePlayer(const PlayerState& state);

		/// Removes a currently active player. Returns false (no-op) if `playerId`
		/// is not currently active.
		bool RemovePlayer(PlayerId playerId);

		/// Every currently active player's latest state, in unspecified order.
		std::vector<PlayerState> Snapshot() const;

		/// The number of currently active players.
		std::size_t PlayerCount() const;

	private:
		std::unordered_map<PlayerId, PlayerState> m_Players;
		// Starts at 1 so 0 stays available as an "unassigned" sentinel elsewhere.
		PlayerId m_NextPlayerId = 1;
	};
}
