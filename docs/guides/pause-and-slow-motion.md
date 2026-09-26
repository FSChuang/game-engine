# Pause & Slow Motion

## Goal

Pause and resume your game, and switch its speed between 0.5×, 1×, and 2×.

## Prerequisites

- An `Application` already constructed and running (see the
  [Quick Start](../index.md#quick-start)).
- Your update callback already reads `deltaTime` and uses it to move things
  (see the [Application Loop concept](../concepts/application-loop.md)).

## Try it immediately — no code required

`Application` already wires up debug keys for exactly this: press **P** to
pause/unpause, and **1** / **2** / **3** to set scale to 0.5× / 1× / 2×. If
your game's update callback already uses the `deltaTime` it's given, you'll
see the effect immediately.

The rest of this guide is for triggering the same behavior from your *own*
code — a pause menu, a slow-motion power-up, and so on.

## Minimal example

```cpp
Engine::Timeline& timeline = application.GetGameTimeline();

// Pause / resume:
timeline.Pause();
timeline.Unpause();

// Speed:
timeline.SetScale(0.5); // half speed
timeline.SetScale(1.0); // normal speed
timeline.SetScale(2.0); // double speed
```

## Step-by-step

1. **Get the game timeline.** `Application::GetGameTimeline()` returns the
   one `Timeline` instance `Run()` samples every frame — this is the
   timeline to change, not a new one you construct yourself.
2. **Pause with `Timeline::Pause`, resume with `Unpause()`.** Call this from
   wherever your own pause-menu logic lives — a key press, a menu button,
   whatever your game uses.
3. **Change speed with `Timeline::SetScale`.** Pass any positive number —
   `0.5`, `1.0`, `2.0`, or any other value your design calls for.
4. **Nothing else to do.** Every system that already reads its `deltaTime`
   from this timeline is affected the next frame, automatically.

## Why it works

`Application::Run` calls `GameTimeline.GetDeltaTime()` once per frame and
hands the result to your update callback. Pausing or scaling the timeline
changes what that *one* call returns — every system downstream of it that
uses the `deltaTime` it was given is affected, with no per-system special
casing. See [Logical Time](../concepts/logical-time.md) for exactly how a
scale change avoids retroactively altering time that already elapsed.

!!! warning "This only affects code that actually asks Timeline for its delta"
    Pausing or scaling `Timeline` does not pause or slow down anything that
    computes its own delta from a different source — real wall-clock time
    (`std::chrono`, SDL's own timer functions used directly) or any other
    independent clock. Nothing in the engine automatically routes such
    systems through `Timeline` for you. If a system needs to respect
    pause/slow-motion, it has to be *given* this timeline's `deltaTime` —
    the same way your update callback already is.

## Common mistakes

- **Constructing a new `Timeline` instead of using `GetGameTimeline()`.** A
  fresh `Timeline` is a completely independent instance — pausing it does
  nothing to the one `Run()` actually samples.
- **Passing `0` or a negative number to `SetScale`.** This throws
  `std::invalid_argument` and leaves the previous scale unchanged — it does
  not silently clamp to some default.
- **Expecting an unrelated, independently-clocked system to slow down for
  free.** See the warning above — it won't, unless it's written to consult
  this `Timeline`.

## Related APIs

- [`Timeline::Pause` / `Unpause` / `IsPaused`](../reference/timeline.md#timelinepause)
- [`Timeline::SetScale` / `GetScale`](../reference/timeline.md#timelinesetscale)
- [`Application::GetGameTimeline`](../reference/application.md#applicationgetgametimeline)

## Next steps

- Read [Logical Time](../concepts/logical-time.md) for the full mental model.
- Read [Timeline Ownership & Sampling](../architecture/timeline-ownership-and-sampling.md)
  if you're combining this with a parent/child `Timeline` hierarchy.
