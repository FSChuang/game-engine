# PlayerRegistry & ServerDispatch

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Network/PlayerRegistry.h</code>, <code>Engine/Network/ServerDispatch.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">EngineNetworkCore — no ZeroMQ dependency</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">PlayerRegistry has no internal synchronization</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Blocking</span><span class="ge-meta__value">Never</span></span>
</div>

For the mental model, see
[System: Server Dispatch](../systems/networking/server-dispatch.md). This
page is deliberately just the API facts.

## PlayerRegistry API

<!-- Generated from Engine/src/Engine/Network/PlayerRegistry.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. -->

--8<-- "player-registry-api.md"

### PlayerRegistry thread safety

No mutex, no atomics — **not internally thread-safe**. If more than one
thread (e.g. multiple session threads in a server) shares one
`PlayerRegistry` instance, the caller must externally synchronize every
access. See [Example: Spare Parts](../examples/spare-parts.md) for one
project's own external-mutex approach — that mutex is that project's code,
not a feature of `PlayerRegistry` itself.

## ServerDispatch API

<!-- Generated from Engine/src/Engine/Network/ServerDispatch.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. -->

--8<-- "server-dispatch-api.md"

### HandleRequest, in detail

The bootstrap-listener dispatch — accepts new players.

| Request | Result |
|---|---|
| `JOIN` | `AssignPlayer()`; replies `Snapshot(newId, roster)` — or `Error(RegistryFull)` if the registry is full |
| `STATE_UPDATE`, `Leaving = false` | `UpdatePlayer()`; replies `Snapshot(id, roster)` — or `Error(UnknownPlayer)` if `id` was never assigned |
| `STATE_UPDATE`, `Leaving = true` | `RemovePlayer()` (idempotent — succeeds whether or not `id` was still active); replies `Snapshot(id, roster)` |
| Anything else, or malformed | `Error(MalformedRequest)`; registry untouched |

Every `Snapshot` this function builds carries a **neutral, all-zero
`PlatformState`** and an **empty `Peers` list** — this function has no
concept of real-time simulation or peer discovery.

### HandleSessionRequest, in detail

For a dedicated per-client session that already represents exactly one
assigned `expectedPlayerId`. Never accepts `JOIN`.

| Request | Result |
|---|---|
| `STATE_UPDATE`, `State.Id == expectedPlayerId`, `Leaving = false` | `UpdatePlayer()`; replies `Snapshot(expectedPlayerId, roster)` — or `Error(UnknownPlayer)` if the registry no longer recognizes `expectedPlayerId` |
| `STATE_UPDATE`, `State.Id == expectedPlayerId`, `Leaving = true` | `RemovePlayer()` (idempotent); replies `Snapshot(expectedPlayerId, roster)` |
| `STATE_UPDATE`, `State.Id != expectedPlayerId` | `Error(UnknownPlayer)` — **registry left completely untouched**, neither `expectedPlayerId`'s nor the claimed ID's entry is modified |
| `JOIN` / `SNAPSHOT` / `ERROR` / `JOIN_ACCEPTED` / malformed | `Error(MalformedRequest)`; registry untouched |

The `State.Id != expectedPlayerId` case is a deliberate identity check: it
stops one client from silently overwriting another player's state by
sending an ID that isn't its own — the session simply refuses, without
mutating anything.

Both functions always return exactly one reply frame; neither ever throws
or leaves a request unanswered.

## See Also

- [System: Server Dispatch](../systems/networking/server-dispatch.md)
- [Reference: Protocol](protocol.md)
- [Example: Spare Parts](../examples/spare-parts.md)
