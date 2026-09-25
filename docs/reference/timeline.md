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

## Synopsis

```cpp
namespace Engine
{
	class Timeline
	{
	public:
		using AnchorSource = std::function<double()>;

		Timeline();
		explicit Timeline(AnchorSource anchorSource);
		explicit Timeline(Timeline& parent);

		double GetTime() const;
		double GetDeltaTime();

		void Pause();
		void Unpause();
		bool IsPaused() const;

		void SetScale(double scale);
		double GetScale() const;

		void SetTicSize(double ticSize);
		double GetTicSize() const;
	};
}
```

## Constructors

`Timeline()`
:   Anchors to real time, via a monotonic engine time source (SDL's tick
    counter) — **must not be constructed before `SDL_Init` has run.** This is
    the constructor `Application` uses for its game timeline.

`explicit Timeline(AnchorSource anchorSource)`
:   Anchors to an arbitrary monotonic callable. This is the seam that makes
    `Timeline` deterministically testable without depending on real elapsed
    time — every test in `Tests/TimelineTest.cpp` uses this constructor with
    a hand-advanced fake clock.

`explicit Timeline(Timeline& parent)`
:   Anchors to another `Timeline`'s logical time (`parent.GetTime()`).
    `parent` must outlive this `Timeline`. See
    [Timeline Ownership & Sampling](../architecture/timeline-ownership-and-sampling.md)
    for the sampling-order rule this requires.

## `AnchorSource`

```cpp
using AnchorSource = std::function<double()>;
```

Supplies the current anchor time (seconds, for a real-time anchor; a parent
`Timeline`'s local time units, for a child) — must be monotonically
non-decreasing.

## `Timeline::GetTime`

```cpp
double GetTime() const;
```

This `Timeline`'s own accumulated logical time. A **pure getter** — it never
samples the anchor and never changes state, so calling it repeatedly always
returns the same value until something else advances the timeline.

## `Timeline::GetDeltaTime`

```cpp
double GetDeltaTime();
```

Samples the anchor and delivers all logical time elapsed since the last call
to `Timeline::GetDeltaTime` — including any interval already flushed by an
intervening `SetScale`/`SetTicSize`/`Pause` call, so no elapsed time is ever
silently lost. Returns `0` while paused. Call once per update, per timeline.

## `Timeline::Pause` / `Unpause` / `IsPaused`

```cpp
void Pause();
void Unpause();
bool IsPaused() const;
```

`Timeline::Pause` freezes logical time: subsequent `GetDeltaTime()` calls
return `0` regardless of how much anchor time passes, until `Unpause()`.
Anchor time that passes while paused is **discarded, not deferred** — there
is no catch-up jump on unpause. Any interval elapsed but not yet sampled at
the moment `Pause()` is called is flushed first (see `SetScale` below) and
still delivered by the next `GetDeltaTime()` call.

## `SetScale` / `GetScale`

```cpp
void SetScale(double scale);
double GetScale() const;
```

`Timeline::SetScale` changes how fast logical time advances relative to the
anchor (`1.0` = one-to-one, `0.5` = half speed, `2.0` = double speed).
**Throws `std::invalid_argument` if `scale <= 0`**, leaving the previous
value in place. Anchor time already elapsed before the call is credited at
the *old* scale first; only anchor time sampled after this call uses the new
one — see [Logical Time](../concepts/logical-time.md#why-a-rate-change-never-rewrites-history)
for the exact mechanism and a worked example.

## `SetTicSize` / `GetTicSize`

```cpp
void SetTicSize(double ticSize);
double GetTicSize() const;
```

The anchor-time duration one local logical-time unit represents — a unit
conversion, not a speed multiplier (see
[Logical Time](../concepts/logical-time.md#the-pipeline)). **Throws
`std::invalid_argument` if `ticSize <= 0`**, leaving the previous value in
place. Follows the same never-retroactive rule as `SetScale`.

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
