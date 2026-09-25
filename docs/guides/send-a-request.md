# Send a Request

## Goal

Send one request over REQ/REP using `Engine::Socket` and `Engine::Protocol`,
and decode the reply.

## Prerequisites

- Something listening on the other end with a `Reply`-role `Socket`,
  calling `Engine::HandleRequest` (or `HandleSessionRequest`) on whatever it
  receives — see
  [System: Server Dispatch](../systems/networking/server-dispatch.md). This
  guide covers the client side only.
- `EngineNetwork` available (cppzmq found by CMake).

## Minimal example

`JoinRequest` is the smallest valid request in the protocol — no payload —
so it's the cleanest illustration:

```cpp
#include "Engine/Network/Protocol.h"
#include "Engine/Network/Socket.h"

Engine::Socket client(Engine::SocketRole::Request);
client.Connect("tcp://127.0.0.1:5556");

std::vector<std::uint8_t> requestBytes = Engine::EncodeJoinRequest();
client.Send(std::string(requestBytes.begin(), requestBytes.end()));

std::string replyFrame = client.Receive(); // blocks until the reply arrives

std::vector<std::uint8_t> replyBytes(replyFrame.begin(), replyFrame.end());
std::optional<Engine::MessageType> type = Engine::PeekMessageType(replyBytes);

if (type == Engine::MessageType::Snapshot)
{
    std::optional<Engine::Snapshot> snapshot = Engine::DecodeSnapshot(replyBytes);
    // snapshot->RecipientId is this client's newly assigned PlayerId.
}
else if (type == Engine::MessageType::Error)
{
    std::optional<Engine::ErrorResponse> error = Engine::DecodeError(replyBytes);
    // error->Code explains why — e.g. ErrorCode::RegistryFull.
}
```

## Step-by-step

1. **Create a `Request`-role `Socket`.** `Engine::Socket client(Engine::SocketRole::Request);`
2. **`Connect` to the server's endpoint.** A `Request` socket connects; it
   never binds — see [Reference: Socket](../reference/socket.md#socketconnect).
3. **Encode the request.** `Engine::EncodeJoinRequest()` returns
   `std::vector<std::uint8_t>` — every `Encode*` function does. `Socket::Send`
   takes a `std::string`, so convert the bytes into one.
4. **`Send` it.** Exactly one frame.
5. **`Receive` the reply.** This **blocks** the calling thread until the
   server replies — there is no timeout.
6. **`PeekMessageType`, then call the matching `Decode*`.** A reply to any
   client request is always either `Snapshot` or `ErrorResponse` — never
   assume which one without checking.

## The server side needs a Reply socket + dispatch logic

This guide is a transport/API example, not a complete multiplayer
architecture. For a request to get an actual reply, something on the other
end needs a `Reply`-role `Socket` bound to that endpoint, reading a
request, calling `Engine::HandleRequest` (or `HandleSessionRequest`) with
its own `PlayerRegistry`, and sending back exactly the bytes that function
returns — see
[Reference: PlayerRegistry & ServerDispatch](../reference/player-registry-and-server-dispatch.md).

!!! warning "Receive() blocks — there is no timeout"
    If nothing ever replies, this call never returns. See
    [Architecture: Networking Threading Model](../architecture/networking-threading-model.md)
    for why this matters for shutdown design, and one pattern (a dedicated
    worker thread) for keeping a blocking `Receive()` off your main thread.

## Related APIs

- [`Socket::Connect` / `Send` / `Receive`](../reference/socket.md)
- [`EncodeJoinRequest` / `PeekMessageType` / `DecodeSnapshot` / `DecodeError`](../reference/protocol.md)

## Next steps

- Read [Reference: Protocol](../reference/protocol.md) for every message
  shape and exact wire layout.
- Read [Reference: Socket](../reference/socket.md) for the full API.
- Read [Example: Spare Parts](../examples/spare-parts.md) for how a real
  project composes this into a full client/server exchange.
