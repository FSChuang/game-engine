#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace Engine
{
	// Uniquely identifies one connected player for the lifetime of a server process.
	using PlayerId = std::uint32_t;

	// Deliberate assignment-level simplification (Milestone 2 Section 2): a small fixed
	// roster cap keeps the wire format free of variable-length framing concerns. This is
	// not a general multiplayer player-count limit, just what this assignment needs
	// ("at least 3 simultaneous clients").
	constexpr std::size_t MaxPlayers = 8;

	// One player's networked state: only what Section 2 needs to reflect movement
	// between clients (ENGINEERING_SPEC.md §5: pure data carrier, public fields, no
	// behavior — same style as Vector2/Color/WindowConfig).
	struct PlayerState
	{
		PlayerId Id;
		float PositionX;
		float PositionY;
		float VelocityX;
		float VelocityY;
	};

	// The leading byte of every encoded message (ENGINEERING_SPEC.md §1: no magic
	// numbers — an explicit, named message-type tag instead of a bare byte literal).
	enum class MessageType : std::uint8_t
	{
		Join = 1,
		StateUpdate = 2,
		Snapshot = 3,
		Error = 4,
	};

	// Failure reasons the server can report back to a client in place of a Snapshot
	// (Milestone 2 Section 2). Deliberately just a code — no message string, no error
	// hierarchy, no diagnostics over the wire.
	enum class ErrorCode : std::uint8_t
	{
		MalformedRequest = 1,
		RegistryFull = 2,
		UnknownPlayer = 3,
	};

	// client -> server: requests a new player ID. No payload beyond the message type.
	struct JoinRequest
	{
	};

	// client -> server: reports the sender's own latest state. `Leaving` is set on a
	// clean disconnect so the server can remove the player from its roster.
	struct StateUpdate
	{
		PlayerState State;
		bool Leaving;
	};

	// server -> client: the recipient's own ID (assigned once at JOIN, then echoed on
	// every later reply) plus every currently active player's state. The reply to a
	// JOIN and the reply to a STATE_UPDATE share this one wire shape (KISS: one shape
	// instead of a separate WELCOME message).
	struct Snapshot
	{
		PlayerId RecipientId;
		std::vector<PlayerState> Roster;
	};

	// server -> client: a Snapshot could not be produced; see ErrorCode. Not used to
	// represent gameplay-level failures with a fake PlayerId — this is the explicit,
	// distinct wire shape for "no Snapshot was produced."
	struct ErrorResponse
	{
		ErrorCode Code;
	};

	// --- Encoding ---
	// Explicit field-by-field byte layout: fixed-width integers in big-endian order,
	// floats as their IEEE-754 bit pattern (via std::memcpy, never reinterpret_cast),
	// also written big-endian. Never a raw memcpy/reinterpret_cast of a whole struct,
	// so the wire format never depends on compiler padding, alignment, or object layout.

	std::vector<std::uint8_t> EncodeJoinRequest();
	std::vector<std::uint8_t> EncodeStateUpdate(const StateUpdate& update);

	// Returns std::nullopt if snapshot.Roster.size() exceeds MaxPlayers, rather than
	// producing a message no decoder could ever accept.
	std::optional<std::vector<std::uint8_t>> EncodeSnapshot(const Snapshot& snapshot);

	std::vector<std::uint8_t> EncodeError(ErrorCode code);

	// --- Decoding ---
	// Returns std::nullopt for any malformed, truncated, padded, wrong-message-type, or
	// oversized-roster input. Never throws, never reads out of bounds.

	std::optional<JoinRequest> DecodeJoinRequest(const std::vector<std::uint8_t>& bytes);
	std::optional<StateUpdate> DecodeStateUpdate(const std::vector<std::uint8_t>& bytes);
	std::optional<Snapshot> DecodeSnapshot(const std::vector<std::uint8_t>& bytes);
	std::optional<ErrorResponse> DecodeError(const std::vector<std::uint8_t>& bytes);

	// Reads just the leading message-type byte, without validating the rest of the
	// frame. Lets a dispatcher decide which specific Decode*() to call; each Decode*()
	// still independently re-checks the type byte itself, so this is a convenience for
	// dispatch, not a trust boundary. Returns std::nullopt for an empty buffer or an
	// unrecognized leading byte.
	std::optional<MessageType> PeekMessageType(const std::vector<std::uint8_t>& bytes);
}
