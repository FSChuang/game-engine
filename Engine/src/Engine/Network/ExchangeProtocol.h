#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace Engine
{
	// A small, generic (game-agnostic) wire format for exchanging a list of networked
	// objects' state (CSC 581 performance checkpoint): used to compare a Full-State
	// strategy (every exchange carries every object) against a Delta-State strategy
	// (every exchange carries only the objects that changed) over the SAME transport and
	// the SAME per-object encoding — the record COUNT per frame is the actual
	// independent variable the two strategies differ on, not the byte layout of a single
	// record. Deliberately knows nothing about players, platforms, or any Spare
	// Parts-specific concept (CLAUDE.md's engine/game boundary) — callers decide what
	// "changed" means and which objects to include.

	/// One object's networked state: a stable network identity, position, and velocity.
	/// Generic — no game-specific meaning, same shape as Engine::PlayerState but not tied
	/// to "player".
	struct ExchangeObjectState
	{
		std::uint32_t Id;
		float PositionX;
		float PositionY;
		float VelocityX;
		float VelocityY;
	};

	/// Bounds every exchange frame this protocol encodes — mirrors Protocol.h's own
	/// MaxPlayers bound, just sized for this experiment's object-count scaling instead of
	/// a player roster.
	constexpr std::size_t MaxExchangeObjects = 2000;

	/// The leading byte of every encoded frame. FullState and DeltaState frames share the
	/// exact same record encoding below this tag — the tag is what tells a receiver
	/// whether to REPLACE its entire tracked object set with the frame's records
	/// (FullState) or MERGE the frame's records into whatever it already has
	/// (DeltaState). That receiver-side behavior, not the byte layout, is the real
	/// difference between the two strategies.
	enum class ExchangeStrategy : std::uint8_t
	{
		FullState = 1,
		DeltaState = 2
	};

	/// A decoded exchange frame: which strategy produced it, a monotonically increasing
	/// sequence number (lets a receiver detect dropped/out-of-order frames), and the
	/// records it carries — every object for FullState, only the changed ones for
	/// DeltaState.
	struct ExchangeFrame
	{
		ExchangeStrategy Strategy;
		std::uint32_t IterationSeq;
		std::vector<ExchangeObjectState> Objects;
	};

	// --- Encoding ---
	// Explicit field-by-field byte layout, same convention as Protocol.cpp: fixed-width
	// integers in big-endian order, floats as their IEEE-754 bit pattern (via
	// std::memcpy, never reinterpret_cast). Never a raw memcpy/reinterpret_cast of a
	// whole struct, so the wire format never depends on compiler padding/alignment.

	/// Encodes a FullState or DeltaState frame from exactly the objects the caller
	/// passes in — for FullState that should be every tracked object, for DeltaState
	/// only the ones that changed since the last frame; this function does not decide
	/// which objects belong in `objects`, it only encodes whichever list it is given.
	/// Returns std::nullopt if objects.size() exceeds MaxExchangeObjects, rather than
	/// producing a frame no decoder could ever accept.
	std::optional<std::vector<std::uint8_t>> EncodeExchangeFrame(ExchangeStrategy strategy, std::uint32_t iterationSeq,
	                                                              const std::vector<ExchangeObjectState>& objects);

	// --- Decoding ---
	// Returns std::nullopt for any malformed, truncated, padded, wrong-message-type, or
	// oversized-record-count input. Never throws, never reads out of bounds.

	/// Returns std::nullopt for malformed, truncated, padded, wrong-message-type, or
	/// oversized-record-count input.
	std::optional<ExchangeFrame> DecodeExchangeFrame(const std::vector<std::uint8_t>& bytes);
}
