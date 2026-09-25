# Time

`Timeline` separates a game's own logical time from real wall-clock time.
Everything about pausing, slow motion, and fast-forward in this engine comes
down to one idea: code that wants to be affected by those things asks a
`Timeline` for its `deltaTime`, instead of reading the clock itself.

## Why Timeline exists

A naive game loop reads `deltaTime` straight from the wall clock. That works
until you need to pause the game, or run it in slow motion, or fast-forward
— at which point every system that reads the clock directly needs its own
special-cased pause/scale logic. `Timeline` exists so that logic lives in
exactly one place: change the `Timeline`, and every piece of code that
already asks it for `deltaTime` is affected automatically.

## What it controls

- **Whether logical time advances at all** (`Pause()` / `Unpause()`)
- **How fast it advances relative to real time** (`SetScale()` — 0.5×, 1×, 2×, or any positive value)
- **What one unit of logical time corresponds to in anchor time** (`SetTicSize()`)
- **A running total** (`GetTime()`) and **newly-elapsed logical time since last asked** (`GetDeltaTime()`)

## What it does NOT control

`Timeline` has no idea that `Renderer`, `PhysicsSystem`, or anything else
exists. Pausing a `Timeline` does not pause rendering, input polling, or any
system that computes its own delta from a different source — it only changes
what that *one* `Timeline` instance reports the next time something asks it.
See the [Pause & Slow Motion guide](../guides/pause-and-slow-motion.md) for
exactly why this distinction matters.

## Who owns the primary game Timeline

`Application` owns one `Timeline` — accessible via `GetGameTimeline()` — for
its entire lifetime, constructed with the default, real-time anchor. This is
the timeline `Run()` samples every frame to produce the `deltaTime` your
update callback receives.

## Standalone and parent Timelines

`Timeline` doesn't require an `Application` at all. It has three
constructors: the default (real-time-anchored) one `Application` uses, one
that anchors to an injected callable (this is what makes `Timeline` itself
unit-testable without real elapsed time), and one that anchors to *another*
`Timeline`'s logical time — a parent/child relationship where pausing or
scaling the parent propagates to the child automatically, and the child can
still apply its own additional scale on top. See
[Timeline Ownership & Sampling](../architecture/timeline-ownership-and-sampling.md)
for the exact rules this composition follows.

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — Timeline</p>

<dl>
<dt>Purpose</dt>
<dd>Logical time, decoupled from wall-clock time: pause, scale, and tic size, all explicit.</dd>

<dt>Owner</dt>
<dd><code>Application</code> owns the primary game timeline. A standalone or parent/child <code>Timeline</code> may be owned by anything — the class has no opinion.</dd>

<dt>Thread Affinity</dt>
<dd>Single-threaded, by omission rather than by explicit design: there is no internal synchronization anywhere in <code>Timeline</code>, so concurrent calls from multiple threads would race on its internal state. Not documented or tested as thread-safe.</dd>

<dt>Blocking Behavior</dt>
<dd>None. Every method is pure computation — no I/O, no waiting.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Not restricted — no deleted copy/move, no non-copyable members (its <code>AnchorSource</code> is a <code>std::function</code>). Copying is possible but not exercised anywhere in this codebase today.</dd>

<dt>Stateful</dt>
<dd>Yes — current logical time, a pending-delta buffer, scale, tic size, and pause state.</dd>

<dt>External Dependencies</dt>
<dd>None directly. The default constructor's anchor reads <code>SDL_GetTicks()</code>, so it must not be constructed before <code>SDL_Init</code> has run.</dd>

<dt>Lifetime</dt>
<dd>No RAII concerns — it owns no external resource, just plain data and a <code>std::function</code>.</dd>

<dt>Public Entry Points</dt>
<dd><code>GetTime</code>, <code>GetDeltaTime</code>, <code>Pause</code>, <code>Unpause</code>, <code>IsPaused</code>, <code>SetScale</code>/<code>GetScale</code>, <code>SetTicSize</code>/<code>GetTicSize</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd><code>SetScale</code>/<code>SetTicSize</code> throw <code>std::invalid_argument</code> for a value ≤ 0, leaving the previous value in place. Not thread-safe — see Thread Affinity.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Logical Time](../concepts/logical-time.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">The clearest explanation of how anchor time becomes logical time — scale, tic size, and why a rate change never rewrites history.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: Timeline](../reference/timeline.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — every constructor and method, with input validation called out.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Architecture: Timeline Ownership & Sampling](../architecture/timeline-ownership-and-sampling.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Parent/child composition, anchor sampling order, and the pending-delta mechanism.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Guide: Pause & Slow Motion](../guides/pause-and-slow-motion.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Task-oriented: pause the game, and switch between 0.5×/1×/2×.</p>
</div>

</div>
