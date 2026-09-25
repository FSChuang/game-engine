// Focused unit tests for Engine's network protocol encode/decode (ENGINEERING_SPEC.md
// §10). Pure logic, no sockets, no SDL. Floats round-trip through their exact IEEE-754
// bit pattern, so exact equality is valid here — no epsilon comparison needed.

#include "Engine/Network/Protocol.h"

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

static Engine::PlayerState MakeState(Engine::PlayerId id, float positionX, float positionY, float velocityX,
                                      float velocityY)
{
	return Engine::PlayerState{ id, positionX, positionY, velocityX, velocityY };
}

static Engine::PlatformState MakePlatform(float positionX, float positionY, float velocityX, float velocityY)
{
	return Engine::PlatformState{ positionX, positionY, velocityX, velocityY };
}

static bool StatesEqual(const Engine::PlayerState& a, const Engine::PlayerState& b)
{
	return a.Id == b.Id && a.PositionX == b.PositionX && a.PositionY == b.PositionY &&
	       a.VelocityX == b.VelocityX && a.VelocityY == b.VelocityY;
}

static bool PlatformsEqual(const Engine::PlatformState& a, const Engine::PlatformState& b)
{
	return a.PositionX == b.PositionX && a.PositionY == b.PositionY && a.VelocityX == b.VelocityX &&
	       a.VelocityY == b.VelocityY;
}

static Engine::PeerInfo MakePeer(Engine::PlayerId id, std::uint16_t p2pPort)
{
	return Engine::PeerInfo{ id, p2pPort };
}

static bool PeersEqual(const Engine::PeerInfo& a, const Engine::PeerInfo& b)
{
	return a.Id == b.Id && a.P2pPort == b.P2pPort;
}

// Finds the entry with a given PlayerId, regardless of vector order — Peers (like
// Roster) is documented as unspecified order, so tests must never assume index i in
// the encoded input lands at index i after decoding.
static const Engine::PeerInfo* FindPeer(const std::vector<Engine::PeerInfo>& peers, Engine::PlayerId id)
{
	for (const Engine::PeerInfo& peer : peers)
	{
		if (peer.Id == id)
		{
			return &peer;
		}
	}
	return nullptr;
}

int main()
{
	// JOIN round-trip.
	{
		std::vector<std::uint8_t> encoded = Engine::EncodeJoinRequest();
		std::optional<Engine::JoinRequest> decoded = Engine::DecodeJoinRequest(encoded);
		Check(decoded.has_value(), "JoinRequest_RoundTrip_Succeeds");
	}

	// JOIN_ACCEPTED round-trip, preserving all three fields exactly (Milestone 2
	// Section 4's AssignedId/AssignedPort, plus Section 5's P2pPort), and matching the
	// documented exact 9-byte wire size.
	{
		Engine::JoinAccepted accepted{ 42, 5561, 6003 };
		std::vector<std::uint8_t> encoded = Engine::EncodeJoinAccepted(accepted);
		std::optional<Engine::JoinAccepted> decoded = Engine::DecodeJoinAccepted(encoded);

		Check(encoded.size() == 9, "JoinAccepted_EncodedSize_IsExactly9Bytes");
		Check(decoded.has_value(), "JoinAccepted_RoundTrip_Succeeds");
		Check(decoded.has_value() && decoded->AssignedId == 42, "JoinAccepted_RoundTrip_PreservesAssignedId");
		Check(decoded.has_value() && decoded->AssignedPort == 5561, "JoinAccepted_RoundTrip_PreservesAssignedPort");
		Check(decoded.has_value() && decoded->P2pPort == 6003, "JoinAccepted_RoundTrip_PreservesP2pPort");
	}

	// JOIN_ACCEPTED rejects wrong type, truncated, and extra-trailing-byte input.
	{
		std::vector<std::uint8_t> wrongType = Engine::EncodeJoinRequest();
		wrongType.resize(9, 0); // JoinAccepted's exact 9-byte wire size, but byte 0 still says JOIN
		Check(!Engine::DecodeJoinAccepted(wrongType).has_value(), "DecodeJoinAccepted_WrongMessageType_Rejected");

		std::vector<std::uint8_t> truncated = Engine::EncodeJoinAccepted(Engine::JoinAccepted{ 1, 1, 1 });
		truncated.resize(truncated.size() - 1);
		Check(!Engine::DecodeJoinAccepted(truncated).has_value(), "DecodeJoinAccepted_Truncated_Rejected");

		std::vector<std::uint8_t> plusExtra = Engine::EncodeJoinAccepted(Engine::JoinAccepted{ 1, 1, 1 });
		plusExtra.push_back(0xFF);
		Check(!Engine::DecodeJoinAccepted(plusExtra).has_value(), "DecodeJoinAccepted_ExtraTrailingByte_Rejected");
	}

	// STATE_UPDATE round-trip, preserving every field exactly.
	{
		Engine::StateUpdate update{ MakeState(7, 100.5f, -40.25f, 12.0f, -3.5f), false };
		std::vector<std::uint8_t> encoded = Engine::EncodeStateUpdate(update);
		std::optional<Engine::StateUpdate> decoded = Engine::DecodeStateUpdate(encoded);

		Check(decoded.has_value(), "StateUpdate_RoundTrip_Succeeds");
		Check(decoded->State.Id == 7, "StateUpdate_RoundTrip_PreservesPlayerId");
		Check(decoded->State.PositionX == 100.5f, "StateUpdate_RoundTrip_PreservesPositionX");
		Check(decoded->State.PositionY == -40.25f, "StateUpdate_RoundTrip_PreservesPositionY");
		Check(decoded->State.VelocityX == 12.0f, "StateUpdate_RoundTrip_PreservesVelocityX");
		Check(decoded->State.VelocityY == -3.5f, "StateUpdate_RoundTrip_PreservesVelocityY");
		Check(decoded->Leaving == false, "StateUpdate_RoundTrip_PreservesLeavingFalse");
	}

	// STATE_UPDATE preserves a true leaving flag distinctly from false.
	{
		Engine::StateUpdate update{ MakeState(3, 0.0f, 0.0f, 0.0f, 0.0f), true };
		std::vector<std::uint8_t> encoded = Engine::EncodeStateUpdate(update);
		std::optional<Engine::StateUpdate> decoded = Engine::DecodeStateUpdate(encoded);

		Check(decoded.has_value() && decoded->Leaving == true, "StateUpdate_RoundTrip_PreservesLeavingTrue");
	}

	// SNAPSHOT round-trip with zero players and a zero PlatformState.
	{
		Engine::Snapshot snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {} };
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		Check(encoded.has_value(), "Snapshot_EncodeEmptyRoster_Succeeds");

		std::optional<Engine::Snapshot> decoded = Engine::DecodeSnapshot(*encoded);
		Check(decoded.has_value() && decoded->RecipientId == 1, "Snapshot_RoundTrip_EmptyRoster_PreservesRecipientId");
		Check(decoded.has_value() && decoded->Roster.empty(), "Snapshot_RoundTrip_EmptyRoster_HasNoPlayers");
		Check(decoded.has_value() && PlatformsEqual(decoded->Platform, snapshot.Platform),
		      "Snapshot_RoundTrip_ZeroPlatformState_PreservedExactly");
	}

	// SNAPSHOT round-trip with one player and a nontrivial PlatformState, including
	// negative velocity.
	{
		Engine::Snapshot snapshot{ 2, MakePlatform(500.25f, -300.5f, -12.5f, 6.75f),
			                        { MakeState(2, 1.0f, 2.0f, 3.0f, 4.0f) } };
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		std::optional<Engine::Snapshot> decoded = Engine::DecodeSnapshot(*encoded);

		Check(decoded.has_value() && decoded->RecipientId == 2, "Snapshot_RoundTrip_OnePlayer_PreservesRecipientId");
		Check(decoded.has_value() && decoded->Roster.size() == 1, "Snapshot_RoundTrip_OnePlayer_HasOneEntry");
		Check(decoded.has_value() && StatesEqual(decoded->Roster[0], snapshot.Roster[0]),
		      "Snapshot_RoundTrip_OnePlayer_PreservesState");
		Check(decoded.has_value() && PlatformsEqual(decoded->Platform, snapshot.Platform),
		      "Snapshot_RoundTrip_OnePlayer_PreservesNontrivialPlatformStateWithNegativeVelocity");
	}

	// SNAPSHOT round-trip with at least 3 players (assignment's minimum client count)
	// and a distinct nontrivial PlatformState.
	{
		Engine::Snapshot snapshot{
			5,
			MakePlatform(-1000.0f, 940.0f, 150.0f, -25.5f),
			{ MakeState(1, 1.0f, 1.0f, 0.0f, 0.0f), MakeState(2, 2.0f, 2.0f, 0.0f, 0.0f),
			  MakeState(3, 3.0f, 3.0f, 0.0f, 0.0f) }
		};
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		std::optional<Engine::Snapshot> decoded = Engine::DecodeSnapshot(*encoded);

		Check(decoded.has_value() && decoded->RecipientId == 5, "Snapshot_RoundTrip_ThreePlayers_PreservesRecipientId");
		Check(decoded.has_value() && decoded->Roster.size() == 3, "Snapshot_RoundTrip_ThreePlayers_HasThreeEntries");
		bool allMatch = decoded.has_value() && decoded->Roster.size() == 3;
		for (std::size_t i = 0; allMatch && i < 3; ++i)
		{
			allMatch = StatesEqual(decoded->Roster[i], snapshot.Roster[i]);
		}
		Check(allMatch, "Snapshot_RoundTrip_ThreePlayers_PreservesEachState");
		Check(decoded.has_value() && PlatformsEqual(decoded->Platform, snapshot.Platform),
		      "Snapshot_RoundTrip_ThreePlayers_PreservesPlatformState");
	}

	// SNAPSHOT supports exactly MaxPlayers entries.
	{
		Engine::Snapshot snapshot{ 9, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {} };
		for (std::size_t i = 0; i < Engine::MaxPlayers; ++i)
		{
			snapshot.Roster.push_back(MakeState(static_cast<Engine::PlayerId>(i + 1), 0.0f, 0.0f, 0.0f, 0.0f));
		}
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		Check(encoded.has_value(), "Snapshot_EncodeMaxPlayers_Succeeds");

		std::optional<Engine::Snapshot> decoded = encoded.has_value() ? Engine::DecodeSnapshot(*encoded)
		                                                               : std::nullopt;
		Check(decoded.has_value() && decoded->Roster.size() == Engine::MaxPlayers,
		      "Snapshot_RoundTrip_MaxPlayers_HasExactlyMaxPlayersEntries");
	}

	// Encoding a roster larger than MaxPlayers is rejected.
	{
		Engine::Snapshot snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {} };
		for (std::size_t i = 0; i < Engine::MaxPlayers + 1; ++i)
		{
			snapshot.Roster.push_back(MakeState(static_cast<Engine::PlayerId>(i + 1), 0.0f, 0.0f, 0.0f, 0.0f));
		}
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		Check(!encoded.has_value(), "Snapshot_EncodeRosterLargerThanMaxPlayers_Rejected");
	}

	// --- Milestone 2 Section 5 engine checkpoint: PeerInfo / Snapshot.Peers ---

	// SNAPSHOT round-trip with an empty peer directory (the neutral placeholder shape
	// Engine's own ServerDispatch always produces — see BuildSnapshotReply).
	{
		Engine::Snapshot snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {}, {} };
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		Check(encoded.has_value(), "Snapshot_EncodeEmptyPeers_Succeeds");

		std::optional<Engine::Snapshot> decoded = encoded.has_value() ? Engine::DecodeSnapshot(*encoded)
		                                                               : std::nullopt;
		Check(decoded.has_value() && decoded->Peers.empty(), "Snapshot_RoundTrip_EmptyPeers_HasNoPeers");
	}

	// SNAPSHOT round-trip with exactly one peer, and a Roster that is deliberately a
	// DIFFERENT size than Peers — proving the two vectors are decoded independently,
	// with no assumption that they share a length or index correlation.
	{
		Engine::Snapshot snapshot{ 2, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f),
			                        { MakeState(2, 1.0f, 2.0f, 3.0f, 4.0f), MakeState(3, 5.0f, 6.0f, 7.0f, 8.0f) },
			                        { MakePeer(9, 6003) } };
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		std::optional<Engine::Snapshot> decoded = encoded.has_value() ? Engine::DecodeSnapshot(*encoded)
		                                                               : std::nullopt;

		Check(decoded.has_value() && decoded->Roster.size() == 2, "Snapshot_RoundTrip_OnePeer_RosterSizeUnaffected");
		Check(decoded.has_value() && decoded->Peers.size() == 1, "Snapshot_RoundTrip_OnePeer_HasOneEntry");
		const Engine::PeerInfo* peer = decoded.has_value() ? FindPeer(decoded->Peers, 9) : nullptr;
		Check(peer != nullptr && PeersEqual(*peer, snapshot.Peers[0]), "Snapshot_RoundTrip_OnePeer_PreservesIdAndPort");
	}

	// SNAPSHOT round-trip with three peers (assignment's minimum client count), each
	// with a distinct nontrivial PlayerId/port pair, found by PlayerId rather than by
	// assumed index — Peers is documented unspecified order, same as Roster.
	{
		Engine::Snapshot snapshot{ 5, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {},
			                        { MakePeer(1, 6001), MakePeer(2, 6002), MakePeer(3, 6003) } };
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		std::optional<Engine::Snapshot> decoded = encoded.has_value() ? Engine::DecodeSnapshot(*encoded)
		                                                               : std::nullopt;

		Check(decoded.has_value() && decoded->Peers.size() == 3, "Snapshot_RoundTrip_ThreePeers_HasThreeEntries");
		bool allMatch = decoded.has_value();
		for (const Engine::PeerInfo& expected : snapshot.Peers)
		{
			const Engine::PeerInfo* actual = decoded.has_value() ? FindPeer(decoded->Peers, expected.Id) : nullptr;
			allMatch = allMatch && actual != nullptr && PeersEqual(*actual, expected);
		}
		Check(allMatch, "Snapshot_RoundTrip_ThreePeers_PreservesEachIdAndPort");
	}

	// SNAPSHOT supports exactly MaxPlayers peer entries.
	{
		Engine::Snapshot snapshot{ 9, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {}, {} };
		for (std::size_t i = 0; i < Engine::MaxPlayers; ++i)
		{
			snapshot.Peers.push_back(
			    MakePeer(static_cast<Engine::PlayerId>(i + 1), static_cast<std::uint16_t>(6001 + i)));
		}
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		Check(encoded.has_value(), "Snapshot_EncodeMaxPlayersPeers_Succeeds");

		std::optional<Engine::Snapshot> decoded = encoded.has_value() ? Engine::DecodeSnapshot(*encoded)
		                                                               : std::nullopt;
		Check(decoded.has_value() && decoded->Peers.size() == Engine::MaxPlayers,
		      "Snapshot_RoundTrip_MaxPlayersPeers_HasExactlyMaxPlayersEntries");
	}

	// Encoding a peer directory larger than MaxPlayers is rejected, exactly like an
	// oversized Roster.
	{
		Engine::Snapshot snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {}, {} };
		for (std::size_t i = 0; i < Engine::MaxPlayers + 1; ++i)
		{
			snapshot.Peers.push_back(MakePeer(static_cast<Engine::PlayerId>(i + 1), 6001));
		}
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		Check(!encoded.has_value(), "Snapshot_EncodePeersLargerThanMaxPlayers_Rejected");
	}

	// Malformed peer count (exceeds MaxPlayers) is rejected even paired with a buffer
	// length matching the (invalid) declared count. For an empty-roster,
	// single-peer Snapshot, the peer-count byte sits right before that one peer's bytes.
	{
		std::vector<std::uint8_t> malformed =
		    *Engine::EncodeSnapshot(Engine::Snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {}, { MakePeer(1, 6001) } });
		std::size_t peerCountIndex = malformed.size() - 1 - 6; // 6 bytes of the one PeerInfo, count byte just before
		malformed[peerCountIndex] = static_cast<std::uint8_t>(Engine::MaxPlayers + 1);
		Check(!Engine::DecodeSnapshot(malformed).has_value(), "DecodeSnapshot_PeerCountExceedsMaxPlayers_Rejected");
	}

	// Truncated SNAPSHOT: peer count declares one peer, but the buffer is cut short
	// inside that one PeerInfo's bytes.
	{
		std::optional<std::vector<std::uint8_t>> encoded =
		    Engine::EncodeSnapshot(Engine::Snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {}, { MakePeer(1, 6001) } });
		encoded->resize(encoded->size() - 2); // chop into the middle of the one peer entry
		Check(!Engine::DecodeSnapshot(*encoded).has_value(), "DecodeSnapshot_TruncatedPeerInfo_Rejected");
	}

	// Extra trailing byte after a well-formed peer directory is rejected.
	{
		std::vector<std::uint8_t> plusExtra =
		    *Engine::EncodeSnapshot(Engine::Snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {}, { MakePeer(1, 6001) } });
		plusExtra.push_back(0xFF);
		Check(!Engine::DecodeSnapshot(plusExtra).has_value(), "DecodeSnapshot_ExtraTrailingByteAfterPeers_Rejected");
	}

	// Truncated JOIN (empty buffer) is rejected.
	{
		std::vector<std::uint8_t> empty;
		Check(!Engine::DecodeJoinRequest(empty).has_value(), "DecodeJoinRequest_Empty_Rejected");
	}

	// Truncated STATE_UPDATE (shorter than the fixed wire size) is rejected.
	{
		std::vector<std::uint8_t> encoded = Engine::EncodeStateUpdate({ MakeState(1, 0.0f, 0.0f, 0.0f, 0.0f), false });
		encoded.resize(encoded.size() - 1);
		Check(!Engine::DecodeStateUpdate(encoded).has_value(), "DecodeStateUpdate_Truncated_Rejected");
	}

	// Truncated SNAPSHOT: header declares one player, but the buffer has no player bytes.
	{
		Engine::Snapshot snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f),
			                        { MakeState(1, 0.0f, 0.0f, 0.0f, 0.0f) } };
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		encoded->resize(encoded->size() - 5); // chop into the middle of the one player entry
		Check(!Engine::DecodeSnapshot(*encoded).has_value(), "DecodeSnapshot_Truncated_Rejected");
	}

	// Truncated SNAPSHOT: cut short inside the PlatformState fields themselves (before
	// the roster-count byte is even reached), distinct from truncating inside the roster.
	{
		Engine::Snapshot snapshot{ 1, MakePlatform(1.0f, 2.0f, 3.0f, 4.0f), {} };
		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeSnapshot(snapshot);
		encoded->resize(encoded->size() - 3); // chop into the middle of the last platform float
		Check(!Engine::DecodeSnapshot(*encoded).has_value(), "DecodeSnapshot_TruncatedPlatformState_Rejected");
	}

	// Unknown/wrong message type is rejected by every decoder, even at a size that
	// would otherwise be structurally valid for that decoder.
	{
		std::vector<std::uint8_t> wrongType = Engine::EncodeStateUpdate({ MakeState(1, 0.0f, 0.0f, 0.0f, 0.0f), false });
		wrongType[0] = 0; // 0 is not a valid MessageType
		Check(!Engine::DecodeStateUpdate(wrongType).has_value(), "DecodeStateUpdate_UnknownMessageType_Rejected");

		std::vector<std::uint8_t> snapshotBytesFedToJoin =
		    *Engine::EncodeSnapshot(Engine::Snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {} });
		snapshotBytesFedToJoin.resize(1); // trim to JOIN's expected size, but the type byte says SNAPSHOT
		Check(!Engine::DecodeJoinRequest(snapshotBytesFedToJoin).has_value(),
		      "DecodeJoinRequest_WrongMessageType_Rejected");
	}

	// Malformed roster count (exceeds MaxPlayers) is rejected even if paired with a
	// buffer length that matches the (invalid) declared count. For an empty-roster,
	// empty-peers Snapshot, the peer-count byte (Milestone 2 Section 5: now appended
	// after Roster) is the last byte, so the roster-count byte is the one right before
	// it — no longer simply "the last byte" the way it was before Peers existed.
	{
		std::vector<std::uint8_t> malformed =
		    *Engine::EncodeSnapshot(Engine::Snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {}, {} });
		std::size_t rosterCountIndex = malformed.size() - 2;
		malformed[rosterCountIndex] = static_cast<std::uint8_t>(Engine::MaxPlayers + 1);
		Check(!Engine::DecodeSnapshot(malformed).has_value(), "DecodeSnapshot_RosterCountExceedsMaxPlayers_Rejected");
	}

	// Extra/inconsistent trailing bytes are rejected for every message type.
	{
		std::vector<std::uint8_t> joinPlusExtra = Engine::EncodeJoinRequest();
		joinPlusExtra.push_back(0xFF);
		Check(!Engine::DecodeJoinRequest(joinPlusExtra).has_value(), "DecodeJoinRequest_ExtraTrailingByte_Rejected");

		std::vector<std::uint8_t> statePlusExtra =
		    Engine::EncodeStateUpdate({ MakeState(1, 0.0f, 0.0f, 0.0f, 0.0f), false });
		statePlusExtra.push_back(0xFF);
		Check(!Engine::DecodeStateUpdate(statePlusExtra).has_value(), "DecodeStateUpdate_ExtraTrailingByte_Rejected");

		std::vector<std::uint8_t> snapshotPlusExtra =
		    *Engine::EncodeSnapshot(Engine::Snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {} });
		snapshotPlusExtra.push_back(0xFF);
		Check(!Engine::DecodeSnapshot(snapshotPlusExtra).has_value(), "DecodeSnapshot_ExtraTrailingByte_Rejected");
	}

	// ERROR round-trip, for every defined code.
	{
		for (Engine::ErrorCode code :
		     { Engine::ErrorCode::MalformedRequest, Engine::ErrorCode::RegistryFull, Engine::ErrorCode::UnknownPlayer })
		{
			std::vector<std::uint8_t> encoded = Engine::EncodeError(code);
			std::optional<Engine::ErrorResponse> decoded = Engine::DecodeError(encoded);
			Check(decoded.has_value() && decoded->Code == code, "Error_RoundTrip_PreservesCode");
		}
	}

	// ERROR rejects truncated, wrong-type, and unrecognized-code input.
	{
		std::vector<std::uint8_t> truncated = Engine::EncodeError(Engine::ErrorCode::MalformedRequest);
		truncated.resize(1);
		Check(!Engine::DecodeError(truncated).has_value(), "DecodeError_Truncated_Rejected");

		std::vector<std::uint8_t> wrongType = Engine::EncodeJoinRequest();
		wrongType.push_back(1); // pad to ERROR's exact size, but the leading byte says JOIN
		Check(!Engine::DecodeError(wrongType).has_value(), "DecodeError_WrongMessageType_Rejected");

		std::vector<std::uint8_t> unknownCode = Engine::EncodeError(Engine::ErrorCode::MalformedRequest);
		unknownCode[1] = 0xFF; // not a defined ErrorCode value
		Check(!Engine::DecodeError(unknownCode).has_value(), "DecodeError_UnrecognizedCode_Rejected");
	}

	// PeekMessageType identifies every valid message's leading byte, and rejects an
	// empty buffer or an unrecognized one.
	{
		Check(Engine::PeekMessageType(Engine::EncodeJoinRequest()) == Engine::MessageType::Join,
		      "PeekMessageType_Join_Identified");
		Check(Engine::PeekMessageType(Engine::EncodeJoinAccepted(Engine::JoinAccepted{ 1, 1, 1 })) ==
		          Engine::MessageType::JoinAccepted,
		      "PeekMessageType_JoinAccepted_Identified");
		Check(Engine::PeekMessageType(Engine::EncodeStateUpdate({ MakeState(1, 0.0f, 0.0f, 0.0f, 0.0f), false })) ==
		          Engine::MessageType::StateUpdate,
		      "PeekMessageType_StateUpdate_Identified");
		Check(Engine::PeekMessageType(
		          *Engine::EncodeSnapshot(Engine::Snapshot{ 1, MakePlatform(0.0f, 0.0f, 0.0f, 0.0f), {} })) ==
		          Engine::MessageType::Snapshot,
		      "PeekMessageType_Snapshot_Identified");
		Check(Engine::PeekMessageType(Engine::EncodeError(Engine::ErrorCode::MalformedRequest)) ==
		          Engine::MessageType::Error,
		      "PeekMessageType_Error_Identified");

		Check(!Engine::PeekMessageType({}).has_value(), "PeekMessageType_Empty_Rejected");
		Check(!Engine::PeekMessageType({ 0xFF }).has_value(), "PeekMessageType_UnrecognizedByte_Rejected");
	}

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
