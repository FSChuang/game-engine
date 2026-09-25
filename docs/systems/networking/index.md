# Networking

The Engine's networking layer is a small set of composable primitives — a
wire protocol, server-side player bookkeeping, request/reply logic, and a
transport wrapper. It is not a multiplayer framework: there is no
connection manager, no client/server topology, and no built-in reconnection
or synchronization policy. Those are decisions every consumer makes for
itself.

## Why two targets

Networking is split into two CMake targets so that its logic never depends
on a real transport to build or test:

- **`EngineNetworkCore`** — pure C++, zero external dependencies beyond the
  standard library. Contains `Protocol`, `PlayerRegistry`, and
  `ServerDispatch`. Builds and unit-tests on any machine, with or without
  ZeroMQ installed.
- **`EngineNetwork`** — the optional transport target. Contains `Socket`,
  the one piece that depends on ZeroMQ (via cppzmq). Skipped entirely by
  CMake if cppzmq isn't found — everything in `EngineNetworkCore` is
  unaffected either way.

See [Concept: Networking Model](../../concepts/networking-model.md) for why
this split matters, not just what it is.

## What each primitive is responsible for

| Primitive | Responsibility |
|---|---|
| **Protocol** | Data shapes and wire encoding/decoding |
| **PlayerRegistry** | Pure server-side player bookkeeping |
| **ServerDispatch** | Request → reply logic over a `PlayerRegistry` |
| **Socket** | RAII ZeroMQ transport wrapper |

## What Engine does NOT provide

None of the following exist anywhere in the Engine's networking layer —
each is a decision left entirely to whatever links these primitives:

- A connection manager
- A game-specific network client (an Engine equivalent of `NetworkClient`)
- A peer-to-peer client (an Engine equivalent of `PeerClient`)
- Any server topology — bootstrap sockets, dedicated sessions, port
  allocation, none of it
- Heartbeat or liveness checking
- Reconnection
- A synchronization/authority policy for multiplayer state
- Multiplayer world authority of any kind

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — Engine Networking Layer</p>

<dl>
<dt>Purpose</dt>
<dd>A wire protocol, server-side bookkeeping, request/reply logic, and a transport wrapper — composable primitives, not a multiplayer framework.</dd>

<dt>Owner</dt>
<dd>No singleton or engine-owned instance of anything here. Every primitive is constructed and owned entirely by whatever consumer links it.</dd>

<dt>Thread Affinity</dt>
<dd><code>Protocol</code> functions and <code>ServerDispatch</code> are pure and safe from any thread. <code>PlayerRegistry</code> has no internal synchronization. <code>Socket</code> is strictly single-thread-owned — see <a href="../../systems/networking/socket.md">System: Socket</a>.</dd>

<dt>Blocking Behavior</dt>
<dd>Only <code>Socket::Receive()</code> blocks (indefinitely, with no timeout). Every other function across all four primitives is non-blocking, synchronous computation.</dd>

<dt>External Dependencies</dt>
<dd><code>EngineNetworkCore</code>: none. <code>EngineNetwork</code>: ZeroMQ, via cppzmq — optional at the build-graph level.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>No topology, connection management, heartbeat, or reconnection exists anywhere in this layer. Consumers build all of that themselves — see <a href="../../examples/spare-parts.md">Example: Spare Parts</a> for one such build.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Networking Model](../../concepts/networking-model.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">The mental model — why the split, and the two socket patterns.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: Protocol](protocol.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Message shapes and encoding principles.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: Socket](socket.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Thread affinity, roles, and the System Contract.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: Server Dispatch](server-dispatch.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">PlayerRegistry and the request/reply rules built on top of it.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Architecture: Networking Threading Model](../../architecture/networking-threading-model.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Why Socket's thread affinity matters, and a consumer pattern.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Example: Spare Parts](../../examples/spare-parts.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">One real composition of these primitives — not the only possible one.</p>
</div>

</div>
