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
}
