#include "Engine/Network/ServerDispatch.h"

namespace Engine
{
	namespace
	{
		// The registry enforces MaxPlayers, so a Snapshot built from its own current
		// contents always encodes successfully — the nullopt case in EncodeSnapshot
		// cannot trigger here.
		std::vector<std::uint8_t> BuildSnapshotReply(const PlayerRegistry& registry, PlayerId recipientId)
		{
			return *EncodeSnapshot(Snapshot{ recipientId, registry.Snapshot() });
		}
	}

	std::vector<std::uint8_t> HandleRequest(PlayerRegistry& registry, const std::vector<std::uint8_t>& requestBytes)
	{
		std::optional<MessageType> type = PeekMessageType(requestBytes);
		if (!type.has_value())
		{
			return EncodeError(ErrorCode::MalformedRequest);
		}

		if (*type == MessageType::Join)
		{
			std::optional<JoinRequest> join = DecodeJoinRequest(requestBytes);
			if (!join.has_value())
			{
				return EncodeError(ErrorCode::MalformedRequest);
			}

			std::optional<PlayerId> newId = registry.AssignPlayer();
			if (!newId.has_value())
			{
				return EncodeError(ErrorCode::RegistryFull);
			}

			return BuildSnapshotReply(registry, *newId);
		}

		if (*type == MessageType::StateUpdate)
		{
			std::optional<StateUpdate> update = DecodeStateUpdate(requestBytes);
			if (!update.has_value())
			{
				return EncodeError(ErrorCode::MalformedRequest);
			}

			if (update->Leaving)
			{
				// Idempotent: whether this player was still active or already gone, the
				// client's desired end state (itself absent from the roster) now holds.
				registry.RemovePlayer(update->State.Id);
				return BuildSnapshotReply(registry, update->State.Id);
			}

			if (!registry.UpdatePlayer(update->State))
			{
				return EncodeError(ErrorCode::UnknownPlayer);
			}

			return BuildSnapshotReply(registry, update->State.Id);
		}

		// SNAPSHOT/ERROR are server -> client only; a client sending either is
		// malformed from the server's point of view.
		return EncodeError(ErrorCode::MalformedRequest);
	}
}
