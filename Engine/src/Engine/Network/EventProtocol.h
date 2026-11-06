#pragma once

#include "Engine/Network/Protocol.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Engine
{
	// A small, explicit, game-agnostic wire format for discrete gameplay-event NOTIFICATIONS
	// sent between peers (Milestone 4 Checkpoint 3). It is the network-side counterpart of
	// the in-process Engine::Event types, deliberately kept separate from them:
	//
	//  - Engine::Event payloads name objects by ObjectId, which is only meaningful inside one
	//    process (every client assigns its own, and remote-player objects are created in
	//    arrival order). Nothing here ever carries an ObjectId — the sender is identified by
	//    its PlayerId, the one identity every endpoint agrees on, and the receiver resolves
	//    that to ITS OWN local object.
	//  - Engine::Event::Timestamp is a local Timeline reading and is meaningless on another
	//    machine's clock. It is never transmitted: the receiver raises the decoded event into
	//    its own EventManager, which stamps it with the receiver's logical time.
	//
	// Same encoding conventions as Protocol.h / ExchangeProtocol.h: explicit field-by-field
	// layout, fixed-width big-endian integers, never a raw struct copy. Delivery is whatever
	// the transport gives: over PUB/SUB a notification can be lost (see PeerClient.h), which
	// is acceptable because these are notifications, not authoritative state.

	/// Leading byte of every event frame. Chosen in the 0x10+ range so it can never be
	/// mistaken for the other formats that share the peer PUB/SUB channel: MessageType uses
	/// 1-5 (a StateUpdate frame starts with 2) and ExchangeStrategy uses 1-2. A receiver
	/// tells the formats apart by this one byte before decoding anything else.
	constexpr std::uint8_t NetworkEventFrameTag = 0x10;

	/// Which gameplay event a frame announces. Add a value here (and its payload, if any) to
	/// network another event kind; a decoder rejects any value it does not know.
	enum class NetworkEventKind : std::uint8_t
	{
		/// The source player died. No payload: an in-process cause (an ObjectId) has no
		/// meaning on another machine, so it is not sent.
		Death = 1,
	};

	/// One decoded notification: what happened and to which player.
	struct NetworkEvent
	{
		NetworkEventKind Kind;
		/// The player the event is about (the sender, for a Death event). Never 0, which is
		/// the reserved "unassigned" PlayerId.
		PlayerId SourcePlayerId;
	};

	/// Encodes an event frame: tag (1) + kind (1) + SourcePlayerId (4) = 6 bytes. Always succeeds.
	std::vector<std::uint8_t> EncodeNetworkEvent(const NetworkEvent& event);

	/// True if the frame's first byte is NetworkEventFrameTag. Says nothing about whether the
	/// rest is valid — a trust-free dispatch helper for a receiver that must route event
	/// frames and state frames differently; DecodeNetworkEvent() still validates everything.
	bool IsNetworkEventFrame(const std::vector<std::uint8_t>& bytes);

	/// Returns std::nullopt for any malformed, truncated, padded, wrong-tag, unknown-kind or
	/// zero-PlayerId input. Never throws, never reads out of bounds.
	std::optional<NetworkEvent> DecodeNetworkEvent(const std::vector<std::uint8_t>& bytes);
}
