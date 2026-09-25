# Server Dispatch

Two pieces work together here: `PlayerRegistry` (pure server-side player
bookkeeping) and `ServerDispatch` (request → reply logic built on top of a
caller-owned `PlayerRegistry`). Neither touches a socket.

## PlayerRegistry

Tracks which players are currently active and their latest reported state
— nothing more.

- **Assigns monotonic `PlayerId`s.** IDs start at 1 and are never reused,
  even after the player that held one is removed.
- **Enforces `MaxPlayers` capacity.** `AssignPlayer()` returns
  `std::nullopt` once that many players are already active.
- **Stores each active player's latest `PlayerState`.**
- **`UpdatePlayer`/`RemovePlayer` are no-ops for an unknown ID** — both
  return `false` rather than inserting a new entry or throwing.
- **`Snapshot()`** returns every currently active player's state, in
  unspecified order.

`PlayerRegistry` is **not internally thread-safe** — there is no mutex, no
atomics, nothing. If more than one server thread shares one
`PlayerRegistry` instance, the caller must externally synchronize every
access to it. `PlayerRegistry` itself does nothing to help with this.

## ServerDispatch

Pure request/reply logic over a `PlayerRegistry&` the caller owns and
passes in — no sockets, no SDL, so the full dispatch behavior is
unit-testable without a real server process.

```cpp
std::vector<std::uint8_t> HandleRequest(PlayerRegistry& registry, const std::vector<std::uint8_t>& requestBytes);

std::vector<std::uint8_t> HandleSessionRequest(PlayerRegistry& registry, PlayerId expectedPlayerId,
                                                const std::vector<std::uint8_t>& requestBytes);
```

**`HandleRequest`** is the bootstrap-listener dispatch — it still accepts
`JOIN` (assigning a new player) as well as `STATE_UPDATE`. **
`HandleSessionRequest`** is for a dedicated per-client session that already
represents one assigned `expectedPlayerId`: it never accepts `JOIN`, and it
rejects a `STATE_UPDATE` whose `State.Id` doesn't match `expectedPlayerId`
— without touching the registry at all — so one client can never overwrite
another player's state by claiming its ID.

Every branch of both functions returns **exactly one** reply frame, so a
`Reply`-role `Socket` calling either once per received request always has
exactly one matching `Send` — the REQ/REP alternation is never left
unsatisfied.

Both functions build every `Snapshot` reply with a **neutral, all-zero
`PlatformState`** and an **empty `Peers` list** — `ServerDispatch` has no
concept of real-time platform simulation or peer discovery at all. A
consumer that wants either fills in the real values itself, after the fact
— see [Example: Spare Parts](../../examples/spare-parts.md) for exactly how.

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: PlayerRegistry & ServerDispatch](../../reference/player-registry-and-server-dispatch.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact signatures, capacity behavior, and every dispatch branch.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Example: Spare Parts](../../examples/spare-parts.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">How a real server synchronizes a shared PlayerRegistry across session threads.</p>
</div>

</div>
