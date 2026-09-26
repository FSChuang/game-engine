# Timeline

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Time/Timeline.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">Core</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">Not thread-safe — no internal synchronization</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Blocking</span><span class="ge-meta__value">Never</span></span>
</div>

For the mental model, see [Logical Time](../concepts/logical-time.md). This
page is deliberately just the API facts.

## Constructors, at a glance

Three constructors, for three different anchor sources — exact signatures
and briefs are in the generated API section below. One detail worth calling
out that the header itself deliberately doesn't mention (it stays
SDL-independent): the default constructor's real-time anchor is SDL's
monotonic tick counter (`SDL_GetTicks()`), so it **must not be constructed
before `SDL_Init` has run** — this is the constructor `Application` uses for
its game timeline. The parent-anchored constructor's sampling-order
requirement is covered in full in
[Timeline Ownership & Sampling](../architecture/timeline-ownership-and-sampling.md).
Every test in `Tests/TimelineTest.cpp` uses the injected-`AnchorSource`
constructor with a hand-advanced fake clock.

## API Reference

<!-- Generated from Engine/src/Engine/Time/Timeline.h by
     scripts/generate_api_docs.py (Documentation Phase 4 pilot) — do not
     hand-edit the section below; edit the header's /// comments instead and
     regenerate. -->

--8<-- "timeline-api.md"

## Behavior notes

`SetScale`/`SetTicSize` never retroactively reinterpret already-elapsed
time — anchor time credited before a rate change keeps its old rate. See
[Logical Time](../concepts/logical-time.md#why-a-rate-change-never-rewrites-history)
for the exact mechanism and a worked example, and
[the pipeline](../concepts/logical-time.md#the-pipeline) for how scale and
tic size compose.

## Thread safety

Not thread-safe. There is no mutex or atomic state anywhere in `Timeline` —
concurrent calls from multiple threads on the same instance would race.
Treat one `Timeline` instance as owned by a single thread, the same way
`Engine::Socket` is.

## Examples

Standalone, deterministic (no real time involved):

```cpp
double fakeTime = 0.0;
Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

fakeTime = 2.0;
double delta = timeline.GetDeltaTime(); // 2.0
```

Parent/child composition:

```cpp
Engine::Timeline parent;     // real-time anchored
Engine::Timeline child(parent);

parent.SetScale(0.5);

// Each frame: sample the parent first, then the child.
parent.GetDeltaTime();
double childDelta = child.GetDeltaTime(); // observes the parent's 0.5x rate
```

## See Also

- [System: Time](../systems/time.md)
- [Concept: Logical Time](../concepts/logical-time.md)
- [Architecture: Timeline Ownership & Sampling](../architecture/timeline-ownership-and-sampling.md)
- [Guide: Pause & Slow Motion](../guides/pause-and-slow-motion.md)
- [Reference: Application](application.md)
