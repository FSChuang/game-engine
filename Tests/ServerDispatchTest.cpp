// Focused unit tests for Engine::HandleRequest (ENGINEERING_SPEC.md §10). Pure logic —
// no sockets, no SDL, no processes — so every server branch is deterministically
// testable in isolation, independent of the real multi-process integration scenario.

#include "Engine/Network/ServerDispatch.h"

#include <cstdio>

static int g_Failures = 0;

static void Check(bool condition, const char* name)
{
	if (condition)
	{
		std::printf("[PASS] %s\n", name);
	}
	else
	{
		std::printf("[FAIL] %s\n", name);
		++g_Failures;
	}
}

static std::optional<Engine::Snapshot> DecodeAsSnapshot(const std::vector<std::uint8_t>& reply)
{
	return Engine::DecodeSnapshot(reply);
}

static std::optional<Engine::ErrorResponse> DecodeAsError(const std::vector<std::uint8_t>& reply)
{
	return Engine::DecodeError(reply);
}

int main()
{
	// JOIN succeeds: assigns an ID, replies with a Snapshot containing just that player.
	{
		Engine::PlayerRegistry registry;
		std::vector<std::uint8_t> reply = Engine::HandleRequest(registry, Engine::EncodeJoinRequest());

		std::optional<Engine::Snapshot> snapshot = DecodeAsSnapshot(reply);
		Check(snapshot.has_value(), "HandleRequest_Join_RepliesWithSnapshot");
		Check(snapshot.has_value() && snapshot->RecipientId != 0, "HandleRequest_Join_AssignsNonZeroId");
		Check(snapshot.has_value() && snapshot->Roster.size() == 1, "HandleRequest_Join_RosterContainsJustSelf");
	}

	// A second JOIN sees the first player already in its roster.
	{
		Engine::PlayerRegistry registry;
		Engine::HandleRequest(registry, Engine::EncodeJoinRequest());
		std::vector<std::uint8_t> secondReply = Engine::HandleRequest(registry, Engine::EncodeJoinRequest());

		std::optional<Engine::Snapshot> snapshot = DecodeAsSnapshot(secondReply);
		Check(snapshot.has_value() && snapshot->Roster.size() == 2,
		      "HandleRequest_SecondJoin_RosterContainsBothPlayers");
	}

	// JOIN when the registry is already full replies with Error(RegistryFull), not a
	// fake Snapshot.
	{
		Engine::PlayerRegistry registry;
		for (std::size_t i = 0; i < Engine::MaxPlayers; ++i)
		{
			Engine::HandleRequest(registry, Engine::EncodeJoinRequest());
		}

		std::vector<std::uint8_t> reply = Engine::HandleRequest(registry, Engine::EncodeJoinRequest());
		std::optional<Engine::ErrorResponse> error = DecodeAsError(reply);
		Check(error.has_value() && error->Code == Engine::ErrorCode::RegistryFull,
		      "HandleRequest_JoinWhenFull_RepliesWithRegistryFullError");
	}

	// STATE_UPDATE for a known player updates the registry and is reflected in the reply.
	{
		Engine::PlayerRegistry registry;
		std::optional<Engine::Snapshot> joinSnapshot =
		    DecodeAsSnapshot(Engine::HandleRequest(registry, Engine::EncodeJoinRequest()));
		Engine::PlayerId id = joinSnapshot->RecipientId;

		Engine::StateUpdate update{ Engine::PlayerState{ id, 42.0f, 7.0f, 1.0f, 2.0f }, false };
		std::vector<std::uint8_t> reply = Engine::HandleRequest(registry, Engine::EncodeStateUpdate(update));

		std::optional<Engine::Snapshot> snapshot = DecodeAsSnapshot(reply);
		Check(snapshot.has_value(), "HandleRequest_StateUpdateKnownPlayer_RepliesWithSnapshot");
		bool foundUpdated = false;
		if (snapshot.has_value())
		{
			for (const Engine::PlayerState& state : snapshot->Roster)
			{
				if (state.Id == id)
				{
					foundUpdated = state.PositionX == 42.0f && state.PositionY == 7.0f && state.VelocityX == 1.0f &&
					               state.VelocityY == 2.0f;
				}
			}
		}
		Check(foundUpdated, "HandleRequest_StateUpdateKnownPlayer_ReflectsNewStateInSnapshot");
	}

	// STATE_UPDATE for a player ID that never joined replies with Error(UnknownPlayer).
	{
		Engine::PlayerRegistry registry;
		Engine::StateUpdate update{ Engine::PlayerState{ 12345, 0.0f, 0.0f, 0.0f, 0.0f }, false };
		std::vector<std::uint8_t> reply = Engine::HandleRequest(registry, Engine::EncodeStateUpdate(update));

		std::optional<Engine::ErrorResponse> error = DecodeAsError(reply);
		Check(error.has_value() && error->Code == Engine::ErrorCode::UnknownPlayer,
		      "HandleRequest_StateUpdateUnknownPlayer_RepliesWithUnknownPlayerError");
	}

	// STATE_UPDATE with Leaving=true removes a known player, and the reply's own
	// snapshot no longer contains it.
	{
		Engine::PlayerRegistry registry;
		Engine::PlayerId id =
		    DecodeAsSnapshot(Engine::HandleRequest(registry, Engine::EncodeJoinRequest()))->RecipientId;

		Engine::StateUpdate leave{ Engine::PlayerState{ id, 0.0f, 0.0f, 0.0f, 0.0f }, true };
		std::vector<std::uint8_t> reply = Engine::HandleRequest(registry, Engine::EncodeStateUpdate(leave));

		std::optional<Engine::Snapshot> snapshot = DecodeAsSnapshot(reply);
		Check(snapshot.has_value(), "HandleRequest_Leaving_StillRepliesWithSnapshot");
		Check(snapshot.has_value() && snapshot->Roster.empty(), "HandleRequest_Leaving_RemovesPlayerFromRoster");
		Check(registry.PlayerCount() == 0, "HandleRequest_Leaving_RegistryCountDrops");
	}

	// STATE_UPDATE with Leaving=true for an already-absent (or never-joined) player is
	// idempotent: it still replies with a valid Snapshot, not an error.
	{
		Engine::PlayerRegistry registry;
		Engine::StateUpdate leave{ Engine::PlayerState{ 999, 0.0f, 0.0f, 0.0f, 0.0f }, true };
		std::vector<std::uint8_t> reply = Engine::HandleRequest(registry, Engine::EncodeStateUpdate(leave));

		std::optional<Engine::Snapshot> snapshot = DecodeAsSnapshot(reply);
		Check(snapshot.has_value(), "HandleRequest_LeavingUnknownPlayer_IsIdempotent_RepliesWithSnapshot");
	}

	// A malformed/empty request replies with Error(MalformedRequest), never crashes,
	// and always produces exactly one reply frame.
	{
		Engine::PlayerRegistry registry;
		std::vector<std::uint8_t> reply = Engine::HandleRequest(registry, {});

		std::optional<Engine::ErrorResponse> error = DecodeAsError(reply);
		Check(error.has_value() && error->Code == Engine::ErrorCode::MalformedRequest,
		      "HandleRequest_EmptyRequest_RepliesWithMalformedRequestError");
	}

	// A server->client-only message type (SNAPSHOT/ERROR) arriving as a "request" is
	// rejected as malformed, not acted on.
	{
		Engine::PlayerRegistry registry;
		std::vector<std::uint8_t> snapshotBytes = *Engine::EncodeSnapshot(Engine::Snapshot{ 1, {} });
		std::vector<std::uint8_t> reply = Engine::HandleRequest(registry, snapshotBytes);

		std::optional<Engine::ErrorResponse> error = DecodeAsError(reply);
		Check(error.has_value() && error->Code == Engine::ErrorCode::MalformedRequest,
		      "HandleRequest_ServerOnlyMessageTypeAsRequest_RepliesWithMalformedRequestError");
		Check(registry.PlayerCount() == 0, "HandleRequest_ServerOnlyMessageTypeAsRequest_DoesNotMutateRegistry");
	}

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
