#include "Engine/Network/ServerDispatch.h"

namespace Engine
{
	namespace
	{
		// Milestone 2 Section 4 checkpoint 1 (protocol only): real server-time platform
		// simulation isn't implemented yet, so every Snapshot reports this neutral,
		// all-zero placeholder until a later checkpoint wires in the actual
		// pure-function-of-steady_clock position (Design 1, no platform thread/mutex).
		constexpr PlatformState NeutralPlatformState{ 0.0f, 0.0f, 0.0f, 0.0f };

		// The registry enforces MaxPlayers, so a Snapshot built from its own current
		// contents always encodes successfully — the nullopt case in EncodeSnapshot
		// cannot trigger here.
		std::vector<std::uint8_t> BuildSnapshotReply(const PlayerRegistry& registry, PlayerId recipientId)
		{
			return *EncodeSnapshot(Snapshot{ recipientId, NeutralPlatformState, registry.Snapshot() });
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

	std::vector<std::uint8_t> HandleSessionRequest(PlayerRegistry& registry, PlayerId expectedPlayerId,
	                                                const std::vector<std::uint8_t>& requestBytes)
	{
		std::optional<MessageType> type = PeekMessageType(requestBytes);
		if (type != MessageType::StateUpdate)
		{
			// A dedicated session belongs to exactly one already-assigned player: it
			// never accepts JOIN (that is bootstrap-only), and any other message
			// direction here is malformed from this session's point of view.
			return EncodeError(ErrorCode::MalformedRequest);
		}

		std::optional<StateUpdate> update = DecodeStateUpdate(requestBytes);
		if (!update.has_value())
		{
			return EncodeError(ErrorCode::MalformedRequest);
		}

		if (update->State.Id != expectedPlayerId)
		{
			// Refuse to let a client claim to be updating a different PlayerId than
			// the one this session was created for. The registry is left completely
			// untouched — neither expectedPlayerId's nor the claimed Id's entry.
			return EncodeError(ErrorCode::UnknownPlayer);
		}

		if (update->Leaving)
		{
			registry.RemovePlayer(expectedPlayerId);
			return BuildSnapshotReply(registry, expectedPlayerId);
		}

		if (!registry.UpdatePlayer(update->State))
		{
			// expectedPlayerId matched the claim, but the registry no longer
			// recognizes it (e.g. already removed) — same error HandleRequest uses
			// for this case.
			return EncodeError(ErrorCode::UnknownPlayer);
		}

		return BuildSnapshotReply(registry, expectedPlayerId);
	}
}
