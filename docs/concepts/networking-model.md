# Networking Model

The Engine's networking layer is split into two physically separate pieces,
built as two separate CMake targets, for one reason: so that the logic —
message shapes, server bookkeeping, request/reply rules — never depends on
a real network transport to build, run, or be tested.

## Transport-independent layer

`Protocol`, `PlayerRegistry`, and `ServerDispatch` together make up
`EngineNetworkCore` — pure C++, zero ZeroMQ dependency, zero sockets.

- **`Protocol`** — data shapes and wire encoding/decoding.
- **`PlayerRegistry`** — pure server-side player bookkeeping.
- **`ServerDispatch`** — request → reply logic built on top of a
  `PlayerRegistry`.

## Transport layer

**`Socket`** — an RAII wrapper around one ZeroMQ context/socket — is the
only piece that depends on ZeroMQ, and lives in the separate, optional
`EngineNetwork` target.

## Why the separation matters

- **Pure logic is testable without ZeroMQ.** Every protocol encode/decode
  case and every dispatch branch is covered by ordinary unit tests that
  never open a socket, never bind a port, and run identically on a machine
  where ZeroMQ isn't even installed.
- **The wire protocol stays explicit.** Nothing about a message's shape or
  byte layout is hidden inside transport code — it's all in one place,
  readable and testable independent of how bytes actually move.
- **The application chooses topology.** Nothing in `EngineNetworkCore`
  assumes a client/server shape, a peer-to-peer shape, or how many
  connections exist. That decision belongs entirely to whatever links
  `EngineNetwork` — see
  [Example: Spare Parts](../examples/spare-parts.md) for one such choice.

## The socket patterns Engine supports

`Socket` curates ZeroMQ down to exactly two patterns:

- **REQ/REP** — request-response discipline. A `Request` socket sends one
  message and blocks for exactly one reply; a `Reply` socket blocks for one
  request and must send exactly one reply before receiving again. Strict
  alternation, one at a time.
- **PUB/SUB** — one-to-many, latest-state/broadcast style. A `Publish`
  socket sends to every currently-connected subscriber with no reply
  expected; a `Subscribe` socket receives whatever a publisher sends,
  independently of any other subscriber.

These are **transport patterns, not game semantics** — REQ/REP doesn't mean
"client/server" and PUB/SUB doesn't mean "peer-to-peer" as a rule baked
into the Engine; they're just two different message-delivery disciplines a
consumer can use for whatever purpose fits. (`ROUTER`/`DEALER` are not
exposed by `Socket` at all — there is no way to reach them through this
wrapper.)

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: Networking Overview](../systems/networking/index.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">What each primitive is responsible for, and what Engine intentionally does not provide.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: Socket](../systems/networking/socket.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Thread affinity, roles, and the System Contract.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Guide: Send a Request](../guides/send-a-request.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">A minimal REQ/REP exchange using Socket and Protocol together.</p>
</div>

</div>
