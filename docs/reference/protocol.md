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

## PlayerId / MaxPlayers

```cpp
using PlayerId = std::uint32_t;
constexpr std::size_t MaxPlayers = 8;
```

`MaxPlayers` bounds both `PlayerRegistry` capacity and every roster/peer
array this protocol encodes — a deliberate fixed cap that keeps the wire
format free of variable-length framing concerns, not a general multiplayer
player-count limit.

## PlayerState

```cpp
struct PlayerState
{
	PlayerId Id;
	float PositionX;
	float PositionY;
	float VelocityX;
	float VelocityY;
};
```

One player's networked position/velocity. Wire size: **20 bytes** (`Id` as
`u32`, then four `float`s, each 4 bytes).

## PlatformState

```cpp
struct PlatformState
{
	float PositionX;
	float PositionY;
	float VelocityX;
	float VelocityY;
};
```

The one server-authoritative moving platform's state — no ID, no array;
exactly one platform. Wire size: **16 bytes** (four `float`s).

## PeerInfo

```cpp
struct PeerInfo
{
	PlayerId Id;
	std::uint16_t P2pPort;
};
```

Peer-discovery/routing data only — how to reach one other player's
peer-to-peer socket, never gameplay state. Wire size: **6 bytes** (`Id` as
`u32`, `P2pPort` as `u16`).

## MessageType / ErrorCode

```cpp
enum class MessageType : std::uint8_t
{
	Join = 1,
	StateUpdate = 2,
	Snapshot = 3,
	Error = 4,
	JoinAccepted = 5,
};

enum class ErrorCode : std::uint8_t
{
	MalformedRequest = 1,
	RegistryFull = 2,
	UnknownPlayer = 3,
};
```

## JoinRequest / JoinAccepted

```cpp
struct JoinRequest {};

struct JoinAccepted
{
	PlayerId AssignedId;
	std::uint16_t AssignedPort;
	std::uint16_t P2pPort;
};
```

`JoinRequest` carries no payload — client → server, "give me an ID." No
`AssignedPort`/`P2pPort` field exists on `JoinRequest`; those come back on
`JoinAccepted`, the reply once dedicated per-client sessions exist.
`AssignedPort` is the dedicated session port; both ports are allocated
outside the Engine (see [Example: Spare Parts](../examples/spare-parts.md)).

## StateUpdate

```cpp
struct StateUpdate
{
	PlayerState State;
	bool Leaving;
};
```

Client → server: the sender's own latest state. `Leaving = true` on a clean
disconnect, so the server removes the player from its roster.

## Snapshot / ErrorResponse

```cpp
struct Snapshot
{
	PlayerId RecipientId;
	PlatformState Platform;
	std::vector<PlayerState> Roster;
	std::vector<PeerInfo> Peers;
};

struct ErrorResponse
{
	ErrorCode Code;
};
```

Server → client: the recipient's own ID, the shared platform's state, every
active player (`Roster`, unspecified order), and the peer directory
(`Peers`, unspecified order, matched to `Roster` by `PlayerId`, not index —
the two vectors are not guaranteed the same length or order).
`ErrorResponse` is the explicit, distinct shape for "no Snapshot was
produced" — see `ErrorCode` above.

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

## Encode\* functions

```cpp
std::vector<std::uint8_t> EncodeJoinRequest();
std::vector<std::uint8_t> EncodeJoinAccepted(const JoinAccepted& accepted);
std::vector<std::uint8_t> EncodeStateUpdate(const StateUpdate& update);
std::optional<std::vector<std::uint8_t>> EncodeSnapshot(const Snapshot& snapshot);
std::vector<std::uint8_t> EncodeError(ErrorCode code);
```

All but `EncodeSnapshot` always succeed. `EncodeSnapshot` returns
`std::nullopt` if `snapshot.Roster.size()` or `snapshot.Peers.size()`
exceeds `MaxPlayers` — rather than producing a message no decoder could
ever accept.

## Decode\* functions

```cpp
std::optional<JoinRequest> DecodeJoinRequest(const std::vector<std::uint8_t>& bytes);
std::optional<JoinAccepted> DecodeJoinAccepted(const std::vector<std::uint8_t>& bytes);
std::optional<StateUpdate> DecodeStateUpdate(const std::vector<std::uint8_t>& bytes);
std::optional<Snapshot> DecodeSnapshot(const std::vector<std::uint8_t>& bytes);
std::optional<ErrorResponse> DecodeError(const std::vector<std::uint8_t>& bytes);
```

Every one of these:

- Rejects (`std::nullopt`) input whose leading byte isn't the exact
  expected `MessageType`.
- Rejects wrong-size input — `DecodeJoinRequest`/`DecodeJoinAccepted`/
  `DecodeStateUpdate`/`DecodeError` require an **exact** byte count;
  `DecodeSnapshot` requires the size implied by its own `rosterCount`/
  `peerCount` bytes, exactly (a declared count too small for the buffer, or
  a buffer with extra trailing bytes, is rejected).
- Rejects `rosterCount`/`peerCount` values greater than `MaxPlayers`, even
  if the buffer length happens to match that oversized declared count.
- `DecodeError` additionally rejects an unrecognized `ErrorCode` byte value.
- Never throws, never reads past the end of `bytes`.

## PeekMessageType

```cpp
std::optional<MessageType> PeekMessageType(const std::vector<std::uint8_t>& bytes);
```

Reads just the leading byte, without validating the rest of the frame —
lets a dispatcher pick which `Decode*` to call. Returns `std::nullopt` for
an empty buffer or an unrecognized leading byte. Each `Decode*` still
independently re-checks the type byte itself, so this is a convenience for
dispatch, never a trust boundary.

## See Also

- [System: Protocol](../systems/networking/protocol.md)
- [Concept: Networking Model](../concepts/networking-model.md)
- [Guide: Send a Request](../guides/send-a-request.md)
- [Reference: PlayerRegistry & ServerDispatch](player-registry-and-server-dispatch.md)
