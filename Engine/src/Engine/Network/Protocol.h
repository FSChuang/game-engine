#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace Engine
{
	/// Uniquely identifies one connected player for the lifetime of a server process.
	using PlayerId = std::uint32_t;

	// Deliberate assignment-level simplification (Milestone 2 Section 2): a small fixed
	// roster cap keeps the wire format free of variable-length framing concerns. This is
	// not a general multiplayer player-count limit, just what this assignment needs
	// ("at least 3 simultaneous clients").
	/// Maximum simultaneous players; bounds every roster/peer array this protocol encodes.
	constexpr std::size_t MaxPlayers = 8;

	/// One player's networked state: position and velocity (ENGINEERING_SPEC.md §5: pure
	/// data carrier, public fields, no behavior — same style as Vector2/Color/WindowConfig).
	struct PlayerState
	{
		PlayerId Id;
		float PositionX;
		float PositionY;
		float VelocityX;
		float VelocityY;
	};

	/// The leading byte of every encoded message (ENGINEERING_SPEC.md §1: no magic
	/// numbers — an explicit, named message-type tag instead of a bare byte literal).
	enum class MessageType : std::uint8_t
	{
		Join = 1,          ///< client -> server: request a new player ID.
		StateUpdate = 2,   ///< client -> server: report the sender's own latest state.
		Snapshot = 3,      ///< server -> client: roster, platform, and peer directory.
		Error = 4,         ///< server -> client: a Snapshot could not be produced.
		JoinAccepted = 5,  ///< server -> client: reply to a successful JOIN.
	};

	/// Failure reasons the server can report back to a client in place of a Snapshot
	/// (Milestone 2 Section 2). Deliberately just a code — no message string, no error
	/// hierarchy, no diagnostics over the wire.
	enum class ErrorCode : std::uint8_t
	{
		MalformedRequest = 1,  ///< The request could not be decoded.
		RegistryFull = 2,      ///< JOIN was rejected: MaxPlayers are already active.
		UnknownPlayer = 3,     ///< A STATE_UPDATE referenced a PlayerId the server does not recognize.
	};

	/// client -> server: requests a new player ID. No payload beyond the message type.
	struct JoinRequest
	{
	};

	// The client derives full endpoints by reusing its bootstrap endpoint's host and
	// substituting these ports — no endpoint string is ever encoded (host is implicit;
	// this project's demo runs every client and the server on one machine).
	/// server -> client: reply to a successful JOIN. Carries the assigned PlayerId, the
	/// dedicated session port, and the peer-to-peer port.
	struct JoinAccepted
	{
		PlayerId AssignedId;
		std::uint16_t AssignedPort;
		std::uint16_t P2pPort;
	};

	// Distinct from PlayerState: this is peer-discovery/routing data, never gameplay
	// state, so it does not belong folded into PlayerState (which client -> server
	// StateUpdate also uses, where a "my own P2P port" field would be meaningless noise
	// sent every tic).
	/// server -> client, embedded in Snapshot: the minimum information one client needs
	/// to reach another's peer-to-peer PUB socket. Host is deliberately not carried —
	/// only a PlayerId and a port.
	struct PeerInfo
	{
		PlayerId Id;
		std::uint16_t P2pPort;
	};

	/// client -> server: reports the sender's own latest state. `Leaving` is set on a
	/// clean disconnect so the server can remove the player from its roster.
	struct StateUpdate
	{
		PlayerState State;
		bool Leaving;
	};

	/// The one server-authoritative moving platform's current state. Exactly one
	/// platform — no ID, no array, no generic replicated-entity concept.
	struct PlatformState
	{
		float PositionX;
		float PositionY;
		float VelocityX;
		float VelocityY;
	};

	// Peers is not a duplicate of Roster: Roster already answers "who is active"
	// (reused as-is), Peers only adds "how do I reach them" (PlayerId -> P2P port).
	// Each PeerInfo carries its own PlayerId rather than relying on index correlation
	// with Roster, since both vectors are in unspecified order and are not guaranteed
	// to be built/ordered the same way.
	/// server -> client: the recipient's own ID, the shared platform's current state,
	/// every currently active player (`Roster`, unspecified order), and the peer
	/// directory needed to reach each of them directly (`Peers`, unspecified order).
	struct Snapshot
	{
		PlayerId RecipientId;
		PlatformState Platform;
		std::vector<PlayerState> Roster;
		std::vector<PeerInfo> Peers;
	};

	/// server -> client: a Snapshot could not be produced; see ErrorCode. Not used to
	/// represent gameplay-level failures with a fake PlayerId — this is the explicit,
	/// distinct wire shape for "no Snapshot was produced."
	struct ErrorResponse
	{
		ErrorCode Code;
	};

	// --- Encoding ---
	// Explicit field-by-field byte layout: fixed-width integers in big-endian order,
	// floats as their IEEE-754 bit pattern (via std::memcpy, never reinterpret_cast),
	// also written big-endian. Never a raw memcpy/reinterpret_cast of a whole struct,
	// so the wire format never depends on compiler padding, alignment, or object layout.

	/// Encodes a JoinRequest. Always succeeds.
	std::vector<std::uint8_t> EncodeJoinRequest();
	/// Encodes a JoinAccepted. Always succeeds.
	std::vector<std::uint8_t> EncodeJoinAccepted(const JoinAccepted& accepted);
	/// Encodes a StateUpdate. Always succeeds.
	std::vector<std::uint8_t> EncodeStateUpdate(const StateUpdate& update);

	/// Returns std::nullopt if snapshot.Roster.size() or snapshot.Peers.size() exceeds
	/// MaxPlayers, rather than producing a message no decoder could ever accept.
	std::optional<std::vector<std::uint8_t>> EncodeSnapshot(const Snapshot& snapshot);

	/// Encodes an ErrorResponse for the given code. Always succeeds.
	std::vector<std::uint8_t> EncodeError(ErrorCode code);

	// --- Decoding ---
	// Returns std::nullopt for any malformed, truncated, padded, wrong-message-type, or
	// oversized-roster input. Never throws, never reads out of bounds.

	/// Returns std::nullopt for malformed, truncated, padded, or wrong-message-type input.
	std::optional<JoinRequest> DecodeJoinRequest(const std::vector<std::uint8_t>& bytes);
	/// Returns std::nullopt for malformed, truncated, padded, or wrong-message-type input.
	std::optional<JoinAccepted> DecodeJoinAccepted(const std::vector<std::uint8_t>& bytes);
	/// Returns std::nullopt for malformed, truncated, padded, or wrong-message-type input.
	std::optional<StateUpdate> DecodeStateUpdate(const std::vector<std::uint8_t>& bytes);
	/// Returns std::nullopt for malformed, truncated, padded, wrong-message-type, or
	/// oversized-roster/peer input.
	std::optional<Snapshot> DecodeSnapshot(const std::vector<std::uint8_t>& bytes);
	/// Returns std::nullopt for malformed, truncated, wrong-message-type, or
	/// unrecognized-code input.
	std::optional<ErrorResponse> DecodeError(const std::vector<std::uint8_t>& bytes);

	/// Reads just the leading message-type byte, without validating the rest of the
	/// frame. Each Decode*() still independently re-checks the type byte itself, so
	/// this is a convenience for dispatch, not a trust boundary. Returns std::nullopt
	/// for an empty buffer or an unrecognized leading byte.
	std::optional<MessageType> PeekMessageType(const std::vector<std::uint8_t>& bytes);
}
