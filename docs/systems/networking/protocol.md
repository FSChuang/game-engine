# Protocol

`Protocol` defines every message the Engine's networking layer knows how to
speak, and the pure functions that turn each one into bytes and back. It has
no transport dependency at all — no sockets, no ZeroMQ — so it builds and
unit-tests everywhere `EngineNetworkCore` does.

## What problem it solves

A client and server need to agree, byte-for-byte, on what a message means —
independent of whatever transport carries it. `Protocol` is that agreement:
a fixed set of message shapes, and encode/decode functions that are the only
code allowed to know the wire layout.

## The message shapes

- **`JoinRequest`** — client → server: "give me an ID." No payload.
- **`JoinAccepted`** — server → client: the assigned `PlayerId`, a dedicated
  session port, and a peer-to-peer port.
- **`StateUpdate`** — client → server: the sender's own latest
  `PlayerState`, plus a `Leaving` flag for clean disconnect.
- **`Snapshot`** — server → client: the recipient's own ID, the shared
  `PlatformState`, the current player roster, and the peer directory
  (`PeerInfo` entries) needed to reach each of them directly.
- **`ErrorResponse`** — server → client: a `Snapshot` could not be produced;
  see `ErrorCode`.

See [Reference: Protocol](../../reference/protocol.md) for every field and
the exact byte layout of each.

## Design principles

- **Explicit fixed-width encoding.** Every integer field has a declared
  width (`u8`/`u16`/`u32`); nothing is encoded as a platform-dependent `int`
  or `size_t`.
- **Big-endian byte order**, independent of host endianness — the wire
  format is the same whether it's read on a big- or little-endian machine.
- **Float bit-pattern preservation.** A `float` is written as its raw
  IEEE-754 bits (via `std::memcpy`), never a whole-struct `memcpy` or
  `reinterpret_cast` — the wire format never depends on compiler padding,
  alignment, or object layout.
- **Exact-size validation.** Every decoder checks that the buffer is
  exactly the size its message type requires (or, for `Snapshot`, the size
  implied by its own declared counts) before reading a single field.
- **Bounded roster/peer counts.** `Roster` and `Peers` are each capped at
  `MaxPlayers` — both encoding and decoding reject anything larger.
- **Malformed input is always rejected, never guessed at.** A decoder
  either fully validates its input and returns a value, or returns
  `std::nullopt` — there is no partial/best-effort decode.

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: Protocol](../../reference/protocol.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Every struct, every Encode/Decode function, and the exact wire layouts.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Networking Model](../../concepts/networking-model.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Why Protocol has no transport dependency, and how it fits with Socket.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Guide: Send a Request](../../guides/send-a-request.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Encode a real request, send it, and decode the reply.</p>
</div>

</div>
