#include "Engine/Network/PlayerRegistry.h"

namespace Engine
{
	std::optional<PlayerId> PlayerRegistry::AssignPlayer()
	{
		if (m_Players.size() >= MaxPlayers)
		{
			return std::nullopt;
		}

		PlayerId newId = m_NextPlayerId++;
		m_Players[newId] = PlayerState{ newId, 0.0f, 0.0f, 0.0f, 0.0f };
		return newId;
	}

	bool PlayerRegistry::UpdatePlayer(const PlayerState& state)
	{
		auto entry = m_Players.find(state.Id);
		if (entry == m_Players.end())
		{
			return false;
		}

		entry->second = state;
		return true;
	}

	bool PlayerRegistry::RemovePlayer(PlayerId playerId)
	{
		return m_Players.erase(playerId) > 0;
	}

	std::vector<PlayerState> PlayerRegistry::Snapshot() const
	{
		std::vector<PlayerState> result;
		result.reserve(m_Players.size());
		for (const auto& entry : m_Players)
		{
			result.push_back(entry.second);
		}
		return result;
	}

	std::size_t PlayerRegistry::PlayerCount() const
	{
		return m_Players.size();
	}
}
