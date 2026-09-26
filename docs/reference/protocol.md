# Protocol

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Network/Protocol.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">EngineNetworkCore — no ZeroMQ dependency</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">Pure functions — safe from any thread</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Blocking</span><span class="ge-meta__value">Never</span></span>
</div>

For the mental model, see [System: Protocol](../systems/networking/protocol.md) and
[Networking Model](../concepts/networking-model.md). This page is deliberately
just the wire facts, using the current code as the sole authority.

## Encoding conventions

- Every message starts with a one-byte `MessageType` tag.
- Multi-byte integers are **big-endian** (`AppendUint32`/`AppendUint16`),
  independent of host endianness.
- Floats are written as their raw IEEE-754 bit pattern (via `std::memcpy`,
  never `reinterpret_cast`), then those 4 bytes are written big-endian —
  never a whole-struct `memcpy`, so the wire format never depends on
  compiler padding or alignment.
- Every `Decode*` function validates **exact** size (or, for `Snapshot`, a
  size computed from its own declared counts) before reading anything, and
  returns `std::nullopt` for anything malformed, truncated, padded, or
  wrong-message-type. None of them throw or read out of bounds.

## API Reference

<!-- Generated from Engine/src/Engine/Network/Protocol.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. Wire-format facts
     (byte layouts, sizes, endianness) are NOT generated -- Doxygen has no
     way to know them -- and stay entirely hand-written below. -->

--8<-- "protocol-api.md"

## Field wire sizes

Byte sizes aren't derivable from the header's comments, so they're kept here
by hand, checked directly against `Protocol.cpp`'s own size constants:

- **`PlayerState`** — 20 bytes (`Id` as `u32`, then four `float`s, each 4 bytes).
- **`PlatformState`** — 16 bytes (four `float`s).
- **`PeerInfo`** — 6 bytes (`Id` as `u32`, `P2pPort` as `u16`).

## Exact wire layouts

**JoinAccepted** — 9 bytes total:

```
[type:u8 = 5]
[assignedId:u32]
[assignedPort:u16]
[p2pPort:u16]
```

**StateUpdate** — 22 bytes total:

```
[type:u8 = 2]
[PlayerState: id:u32, positionX:f32, positionY:f32, velocityX:f32, velocityY:f32]
[leaving:u8 (0 or 1 — any other value is rejected)]
```

**Snapshot** — variable length, `22 + 20×rosterCount + 1 + 6×peerCount` bytes:

```
[type:u8 = 3]
[recipientId:u32]
[PlatformState: positionX:f32, positionY:f32, velocityX:f32, velocityY:f32]
[rosterCount:u8]
[roster entries — rosterCount × PlayerState, 20 bytes each]
[peerCount:u8]
[peer entries — peerCount × PeerInfo, 6 bytes each]
```

**JoinRequest** — 1 byte (`[type:u8 = 1]`, no payload).
**ErrorResponse** — 2 bytes (`[type:u8 = 4][code:u8]`).

## Behavior notes

- **`MaxPlayers`** is a deliberate fixed cap that keeps the wire format free
  of variable-length framing concerns — not a general multiplayer
  player-count limit.
- **`JoinAccepted`'s `AssignedPort`** is the dedicated session port; both it
  and `P2pPort` are allocated outside the Engine (see
  [Example: Spare Parts](../examples/spare-parts.md)).
- **`Snapshot.Peers`** is matched to `Roster` by `PlayerId`, not index — the
  two vectors are not guaranteed the same length or order.
- Every `Decode*` function, collectively:
    - Rejects (`std::nullopt`) input whose leading byte isn't the exact
      expected `MessageType`.
    - Rejects wrong-size input — most require an **exact** byte count;
      `DecodeSnapshot` requires the size implied by its own `rosterCount`/
      `peerCount` bytes, exactly (a declared count too small for the buffer,
      or a buffer with extra trailing bytes, is rejected).
    - Rejects `rosterCount`/`peerCount` values greater than `MaxPlayers`,
      even if the buffer length happens to match that oversized declared
      count.
    - Never throws, never reads past the end of `bytes`.

## See Also

- [System: Protocol](../systems/networking/protocol.md)
- [Concept: Networking Model](../concepts/networking-model.md)
- [Guide: Send a Request](../guides/send-a-request.md)
- [Reference: PlayerRegistry & ServerDispatch](player-registry-and-server-dispatch.md)
