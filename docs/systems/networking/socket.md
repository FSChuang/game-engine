# Socket

`Socket` is a thin RAII wrapper over one ZeroMQ context and socket,
restricted to four roles: `Request`, `Reply`, `Publish`, `Subscribe`. It is
the only piece of the Engine networking layer that depends on ZeroMQ.

## What problem it solves

ZeroMQ's own C++ API (`zmq::context_t`/`zmq::socket_t`) is general-purpose
and exposes many socket types and options this project never uses.
`Socket` curates that down to exactly the four roles this transport needs,
with RAII lifetime and a small, explicit method set — nothing more.

## What it owns

One `zmq::context_t` and one `zmq::socket_t`, for its entire lifetime,
created together in the constructor.

## What it does NOT own

- **No thread.** `Socket` never spawns a thread, never runs a loop, never
  schedules anything. Every method does exactly the one ZeroMQ operation
  it names and returns.
- **No connection manager, reconnect, or heartbeat logic.** A dropped or
  unresponsive peer is invisible to `Socket` — it has no way to detect or
  recover from one.
- **No message framing beyond one ZeroMQ frame.** `Send`/`Receive` operate
  on a single frame each; anything above that (message boundaries within a
  frame, multi-part messages) is the caller's concern, and this project
  doesn't use either.

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

`Request`/`Reply` are ZeroMQ's synchronous REQ/REP pattern.
`Publish`/`Subscribe` are its PUB/SUB pattern — fire-and-forget, one
publisher fanning out to many subscribers. **`ROUTER`/`DEALER` are not
exposed at all** — not a subset users can reach through this wrapper.

## Bind / Connect role rules

`Bind` is valid for `Reply`/`Publish` only; `Connect` is valid for
`Request`/`Subscribe` only. These rules are checked with `ENGINE_ASSERT`,
which **compiles out entirely in release builds** (`NDEBUG`) — in a release
build, calling `Bind`/`Connect` on the wrong role skips that check and
falls straight through to the underlying ZeroMQ call, which may itself
throw `zmq::error_t`, or may simply behave however ZeroMQ defines that
operation for the actual socket type. The role restriction is a documented
contract enforced in debug builds, not a runtime guarantee in every build.

`Subscribe` role sockets may call `Connect` repeatedly to connect to
multiple publishers — unlike `Request`, there is no "connect once"
assumption.

## Public entry points

```cpp
explicit Socket(SocketRole role);

void Bind(const std::string& endpoint);
void Connect(const std::string& endpoint);
void Disconnect(const std::string& endpoint);
void Subscribe(const std::string& topic);

void Send(const std::string& message);
std::string Receive();
std::optional<std::string> TryReceive();
```

## Thread affinity — the most important fact about this class

**One `Socket` instance belongs to exactly one thread, for its entire
lifetime.** This matches cppzmq's own contract (a `zmq::context_t` may be
shared across threads; a `zmq::socket_t` may not) and is not something
`Socket` adds on top — it's a direct consequence of what it wraps.

- `Socket` itself spawns **zero** threads.
- `Socket` has **no internal synchronization** — no mutex, no atomics.
- The caller owns the entire thread model: how many threads exist, which
  thread owns which `Socket`, and how results cross back to any other
  thread.
- **Never pass the same `Socket` instance to be used concurrently from more
  than one thread** — not even for reads. See
  [Architecture: Networking Threading Model](../../architecture/networking-threading-model.md)
  for the full reasoning and a concrete consumer pattern.

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — Socket</p>

<dl>
<dt>Purpose</dt>
<dd>Thin RAII wrapper over one ZeroMQ context + socket, restricted to <code>SocketRole::Request</code> / <code>Reply</code> / <code>Publish</code> / <code>Subscribe</code>.</dd>

<dt>Owner</dt>
<dd>Whichever function/thread constructs it. The class itself has no opinion beyond RAII — it does not assume a particular owner.</dd>

<dt>Thread Affinity</dt>
<dd>Single thread only. Must be created, used, and destroyed on exactly one thread — see above. Not enforced by any assertion or check; violating this is undefined behavior at the ZeroMQ level.</dd>

<dt>Blocking Behavior</dt>
<dd><code>Receive()</code> blocks until a message arrives — indefinitely, if none ever does. <code>TryReceive()</code> never blocks — one <code>ZMQ_DONTWAIT</code> attempt, returning <code>std::nullopt</code> if nothing is available. <code>Bind</code>/<code>Connect</code>/<code>Disconnect</code>/<code>Subscribe</code>/<code>Send</code> return promptly.</dd>

<dt>Resource Ownership</dt>
<dd>Owns one ZeroMQ context and one socket for its entire lifetime. RAII: the destructor closes the socket and terminates the context — there is no separate <code>Close()</code>/<code>Shutdown()</code> call.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Neither copyable nor movable. Copy is explicitly deleted, which also suppresses the implicit move.</dd>

<dt>External Dependencies</dt>
<dd>ZeroMQ, via cppzmq. Part of the optional <code>EngineNetwork</code> target — skipped entirely on a machine without cppzmq installed.</dd>

<dt>Public Entry Points</dt>
<dd><code>Bind</code>, <code>Connect</code>, <code>Disconnect</code>, <code>Subscribe</code>, <code>Send</code>, <code>Receive</code>, <code>TryReceive</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>No receive timeout of any kind — a blocking <code>Receive()</code> with no responding peer blocks forever. <code>Bind</code>/<code>Connect</code>/<code>Send</code>/<code>Receive</code> throw on genuine ZeroMQ failure — most paths throw <code>zmq::error_t</code> (cppzmq's own exception), but <code>Receive()</code>'s own explicit "no message received" check throws <code>std::runtime_error</code> instead; see <a href="../../reference/socket.md">Reference: Socket</a>. <code>TryReceive()</code> is the one method that reports "nothing yet" as <code>std::nullopt</code> rather than an exception.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: Socket](../../reference/socket.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API, including Receive/TryReceive's precise blocking behavior.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Architecture: Networking Threading Model](../../architecture/networking-threading-model.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Why thread affinity matters, and one consumer pattern (not a requirement).</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Networking Model](../../concepts/networking-model.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">REQ/REP vs. PUB/SUB, conceptually.</p>
</div>

</div>
