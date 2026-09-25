#pragma once

#include "Engine/Network/PlayerRegistry.h"
#include "Engine/Network/Protocol.h"

#include <cstdint>
#include <vector>

namespace Engine
{
	// Applies one already-received request frame to `registry` and returns the exact
	// reply frame to send back. Pure logic — no sockets, no SDL — so the full
	// request/reply behavior is unit-testable without a real server process; the
	// headless server executable is the only place that actually binds a socket and
	// calls this once per received request.
	//
	// Dispatch:
	//   JOIN                          -> AssignPlayer(); Snapshot(newId, roster),
	//                                    or Error(RegistryFull) if the registry is full.
	//   STATE_UPDATE (Leaving=false)  -> UpdatePlayer(); Snapshot(id, roster),
	//                                    or Error(UnknownPlayer) if id was never assigned.
	//   STATE_UPDATE (Leaving=true)   -> RemovePlayer() (idempotent: succeeds whether or
	//                                    not id was still active); Snapshot(id, roster).
	//   anything else / malformed     -> Error(MalformedRequest).
	//
	// Every branch returns exactly one reply frame, so a REP socket calling this once
	// per received request always has exactly one matching send — the REQ/REP
	// alternation is never left unsatisfied.
	std::vector<std::uint8_t> HandleRequest(PlayerRegistry& registry, const std::vector<std::uint8_t>& requestBytes);

	// Applies one already-received request frame from a DEDICATED per-client session
	// (Milestone 2 Section 4) that represents exactly one already-assigned
	// `expectedPlayerId` — never a JOIN, and never any PlayerId other than its own.
	// Unlike HandleRequest (the bootstrap listener's dispatch, which still assigns new
	// players), a session must not accept JOIN at all, and must not let a client claim
	// to be updating a different PlayerId than the one this session was created for —
	// that would let one client silently overwrite another player's state.
	//
	// Dispatch:
	//   STATE_UPDATE, State.Id == expectedPlayerId, Leaving=false -> UpdatePlayer();
	//     Snapshot(expectedPlayerId, roster), or Error(UnknownPlayer) if the registry
	//     no longer recognizes expectedPlayerId (e.g. already removed).
	//   STATE_UPDATE, State.Id == expectedPlayerId, Leaving=true  -> RemovePlayer()
	//     (idempotent); Snapshot(expectedPlayerId, roster).
	//   STATE_UPDATE, State.Id != expectedPlayerId -> Error(UnknownPlayer); the
	//     registry is left completely untouched (neither expectedPlayerId's nor the
	//     claimed Id's entry is modified).
	//   JOIN / SNAPSHOT / ERROR / JOIN_ACCEPTED / malformed -> Error(MalformedRequest);
	//     registry untouched.
	//
	// Pure logic — no sockets — so this is unit-testable exactly like HandleRequest.
	// Every branch returns exactly one reply frame.
	std::vector<std::uint8_t> HandleSessionRequest(PlayerRegistry& registry, PlayerId expectedPlayerId,
	                                                const std::vector<std::uint8_t>& requestBytes);
}
