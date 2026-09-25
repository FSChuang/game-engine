# Renderer

`Renderer` owns the SDL window and rendering surface and draws `Entity`
objects as filled, scaled rectangles. It is the one object a game uses to get
pixels on screen — everything about *what* to draw is the game's decision,
made once per frame from inside the render callback `Application::Run` hands
it.

## What problem it solves

Every SDL game needs the same window/renderer setup and teardown, and some
way to map a game's own logical coordinates onto whatever size the window
actually is right now. `Renderer` owns that setup once, and exposes a single
scaling-mode switch so a game can choose — at runtime — whether its
coordinates track the window size or ignore it entirely.

## What it owns

- The raw `SDL_Window*` / `SDL_Renderer*` handles, created together in the
  constructor and destroyed together in the destructor.
- A **reference resolution** — the `WindowConfig` width/height it was
  constructed with, captured once and never updated afterward, even though
  the window itself can be resized.
- Its current `ScalingMode` (`Constant` by default).

## What it does NOT own

- **No `Entity` storage.** `DrawEntity` takes a `const Entity&` and reads it
  once; `Renderer` keeps no list of entities and draws nothing on its own
  initiative.
- **No textures or images.** Every draw is `SDL_RenderFillRect` — a solid
  color rectangle. There is no sprite/texture support yet (SDL3_image is not
  integrated).
- **No input handling.** `Renderer` doesn't know a keyboard exists;
  `ToggleScalingMode`/`SetScalingMode` are pure mutators that something else
  — `Application`, or a game directly — decides when to call.
- **No camera, viewport, or z-ordering.** Entities are drawn in whatever
  order `DrawEntity` is called, with no depth or culling.

## Public entry points

```cpp
void BeginFrame();
void EndFrame();
void DrawEntity(const Entity& entity);

void SetScalingMode(ScalingMode mode);
ScalingMode GetScalingMode() const;
void ToggleScalingMode();
```

`BeginFrame`/`EndFrame` bracket a frame's drawing (clear, then present);
`DrawEntity` is called once per entity in between. The scaling-mode methods
are plain, independent state mutators — see
[Rendering & Scaling](../concepts/rendering-and-scaling.md) for exactly how
that state changes what `DrawEntity` draws.

## Relationship to Application

`Application` owns exactly one `Renderer` (`Scope<Renderer>`) for its entire
lifetime, and drives it every frame: `BeginFrame()`, then the game's render
callback (where `DrawEntity` calls typically happen), then `EndFrame()`. See
the [Application Loop concept](../concepts/application-loop.md) for the full
per-frame order this fits into.

`Application` is also the source of the one built-in scaling-mode trigger:
its `ProcessScalingModeToggle` calls `Renderer::ToggleScalingMode()` when
**Tab** is just-pressed. A game can call `SetScalingMode`/`ToggleScalingMode`
itself too, through the `Renderer&` its render callback receives — see the
[Toggle Scaling Mode guide](../guides/toggle-scaling-mode.md).

## What it does NOT do

No aspect-ratio preservation: `Proportional` mode scales the X and Y axes
**independently**, so resizing to a different aspect ratio than the
reference resolution visibly stretches or squashes every entity. Nothing in
`Renderer` corrects for this.

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — Renderer</p>

<dl>
<dt>Purpose</dt>
<dd>Owns the SDL window/renderer and draws <code>Entity</code> objects as filled, scaled rectangles.</dd>

<dt>Owner</dt>
<dd><code>Application</code> owns one instance via <code>Scope&lt;Renderer&gt;</code>. Nothing requires that ownership — a caller can construct a <code>Renderer</code> standalone.</dd>

<dt>Thread Affinity</dt>
<dd>Single-threaded, by convention rather than enforcement — SDL rendering calls are expected to run on the thread that created the window; nothing in <code>Renderer</code> synchronizes access.</dd>

<dt>Blocking Behavior</dt>
<dd><code>BeginFrame</code>/<code>DrawEntity</code> do not block. <code>EndFrame</code> calls <code>SDL_RenderPresent</code>, which may block on vsync depending on SDL/driver settings — a detail owned by SDL, not by <code>Renderer</code> itself.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Neither copyable nor movable — copy is explicitly deleted, which also suppresses the implicit move.</dd>

<dt>Stateful</dt>
<dd>Yes — the SDL handles, the reference resolution captured at construction, and the current scaling mode.</dd>

<dt>External Dependencies</dt>
<dd>SDL3 (<code>SDL_CreateWindowAndRenderer</code>, <code>SDL_DestroyRenderer</code>/<code>Window</code>, and the per-frame draw calls).</dd>

<dt>Lifetime</dt>
<dd>RAII. The constructor creates the window and renderer together; if either call fails, whichever one succeeded is destroyed before <code>std::runtime_error</code> is thrown — construction never leaves a partial resource behind.</dd>

<dt>Public Entry Points</dt>
<dd><code>Renderer(const WindowConfig&)</code>, <code>BeginFrame</code>, <code>EndFrame</code>, <code>DrawEntity</code>, <code>SetScalingMode</code>/<code>GetScalingMode</code>/<code>ToggleScalingMode</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>Filled-rectangle rendering only — no textures/sprites. <code>Proportional</code> mode scales X/Y independently and can distort shape on a non-uniform resize. Throws <code>std::runtime_error</code> (from <code>SDL_GetError()</code>) if window/renderer creation fails.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Rendering & Scaling](../concepts/rendering-and-scaling.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">The exact scaling math — Constant vs. Proportional, worked examples.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: Renderer](../reference/renderer.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — WindowConfig, ScalingMode, every method.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Guide: Toggle Scaling Mode](../guides/toggle-scaling-mode.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Task-oriented: switch scaling modes from your own code.</p>
</div>

</div>
