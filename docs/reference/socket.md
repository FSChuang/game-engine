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

!!! danger "Thread affinity — read this before using Socket"
    **One `Socket` instance belongs to exactly one thread, for its entire
    lifetime.** It spawns no threads and has no internal synchronization —
    never construct, call a method on, or destroy the same `Socket` from
    more than one thread. This matches cppzmq's own contract (a
    `zmq::context_t` may be shared across threads; a `zmq::socket_t` may
    not). See
    [Architecture: Networking Threading Model](../architecture/networking-threading-model.md)
    for the full reasoning and a concrete consumer pattern — this is not
    something the generated API section below can convey on its own.

## API Reference

<!-- Generated from Engine/src/Engine/Network/Socket.h by
     scripts/generate_api_docs.py (Documentation Phase 4 pilot) — do not
     hand-edit the section below; edit the header's /// comments instead and
     regenerate. -->

--8<-- "socket-api.md"

## Behavior notes

- **`Bind`/`Connect` role rules are `ENGINE_ASSERT`-checked, which compiles
  out entirely in release builds** — see
  [System: Socket](../systems/networking/socket.md#thread-affinity-the-most-important-fact-about-this-class)
  for what that means in practice: the role restriction is a documented
  contract, not a guarantee enforced in every build.
- **`Connect`'s "connect once" behavior for `Request` is a convention, not
  an enforced rule** — nothing in code stops repeated calls, but every real
  use in this project connects a `Request` socket exactly once.
- **`Disconnect`** is valid for any role that has called `Connect` — most
  meaningfully `Subscribe`, which is the only role that routinely connects
  to more than one endpoint.

## See Also

- [System: Socket](../systems/networking/socket.md)
- [Architecture: Networking Threading Model](../architecture/networking-threading-model.md)
- [Guide: Send a Request](../guides/send-a-request.md)
