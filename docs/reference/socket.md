# Socket

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Network/Socket.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">EngineNetwork — requires ZeroMQ</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">Single thread only — see System Contract</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Ownership</span><span class="ge-meta__value">Owns one zmq::context_t + zmq::socket_t</span></span>
</div>

For the mental model, see [System: Socket](../systems/networking/socket.md).
This page is deliberately just the API facts.

## Synopsis

```cpp
namespace Engine
{
	enum class SocketRole
	{
		Request,
		Reply,
		Publish,
		Subscribe
	};

	class Socket
	{
	public:
		explicit Socket(SocketRole role);

		Socket(const Socket&) = delete;
		Socket& operator=(const Socket&) = delete;

		void Bind(const std::string& endpoint);
		void Connect(const std::string& endpoint);
		void Disconnect(const std::string& endpoint);
		void Subscribe(const std::string& topic);

		void Send(const std::string& message);
		std::string Receive();
		std::optional<std::string> TryReceive();
	};
}
```

## Socket

```cpp
explicit Socket(SocketRole role);
```

Constructs a ZeroMQ context and a socket of the type matching `role`
together. Neither copyable nor movable.

## SocketRole

```cpp
enum class SocketRole
{
	Request,    // ZMQ_REQ
	Reply,      // ZMQ_REP
	Publish,    // ZMQ_PUB
	Subscribe   // ZMQ_SUB
};
```

Fixed at construction — there is no way to change a `Socket`'s role
afterward.

## Socket::Bind

```cpp
void Bind(const std::string& endpoint);
```

Valid for `Reply`/`Publish` only (checked via `ENGINE_ASSERT`, which
compiles out in release builds — see
[System: Socket](../systems/networking/socket.md#thread-affinity-the-most-important-fact-about-this-class)
for what that means in practice). Starts listening at `endpoint`, e.g.
`"tcp://127.0.0.1:5555"` for one interface or `"tcp://*:5555"` for all.

## Socket::Connect

```cpp
void Connect(const std::string& endpoint);
```

Valid for `Request`/`Subscribe` only. Connects to a peer already bound at
`endpoint`. A `Subscribe` socket may call this repeatedly to connect to
**multiple** publishers over its lifetime — `Request` has no documented
"connect once" restriction enforced in code, but every real use in this
project connects a `Request` socket exactly once.

## Socket::Disconnect

```cpp
void Disconnect(const std::string& endpoint);
```

Severs one specific connection previously made via `Connect(endpoint)`,
leaving any other connections on this same socket untouched. Valid for any
role that has called `Connect` — most meaningfully `Subscribe`.

## Socket::Subscribe

```cpp
void Subscribe(const std::string& topic);
```

Valid for `Subscribe` role only. Registers interest in messages whose
payload starts with `topic`. An empty string subscribes to everything a
connected publisher sends. A freshly-constructed `Subscribe` socket
receives **nothing** until this is called at least once.

## Socket::Send

```cpp
void Send(const std::string& message);
```

Sends `message` as a single ZeroMQ frame. Valid for `Request`/`Reply` (as
part of their alternating discipline) and `Publish` (fire-and-forget
broadcast to every currently-connected subscriber).

## Socket::Receive

```cpp
std::string Receive();
```

**Blocks the calling thread until exactly one frame arrives** — there is no
timeout of any kind. If no peer ever sends, this call never returns. Throws
`std::runtime_error` in the (not ordinarily expected) case where the
underlying blocking `recv` call itself reports failure; genuine ZeroMQ
failures from the operation throw `zmq::error_t`, same as every other
method here.

## Socket::TryReceive

```cpp
std::optional<std::string> TryReceive();
```

Performs **exactly one non-blocking receive attempt** (`ZMQ_DONTWAIT`) —
never blocks, never internally retries or spins. Returns the message if one
was already fully available; returns `std::nullopt` if none was available
at the moment of the call — this is the expected, non-error outcome, not a
failure. A genuine ZeroMQ error still throws `zmq::error_t`, exactly like
every other method on this class.

## See Also

- [System: Socket](../systems/networking/socket.md)
- [Architecture: Networking Threading Model](../architecture/networking-threading-model.md)
- [Guide: Send a Request](../guides/send-a-request.md)
