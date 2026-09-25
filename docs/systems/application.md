# Application

`Application` owns the SDL lifecycle and the main update/render loop. It is
the one object a game constructs to get a window, an input source, a game
timeline, and a running loop — everything else in a game is built by
composing systems around it, not by extending it.

## What problem it solves

Every SDL game needs the same boilerplate: initialize SDL, create a window
and renderer, poll events every frame, and tear everything down in the right
order on exit. `Application` does that once, correctly, so a game never has
to touch `SDL_Init`/`SDL_Quit` itself.

## What it owns

- A `Renderer` (the window and drawing surface — not yet documented on its own page)
- An `InputManager` (polled keyboard state — not yet documented on its own page)
- A [`Timeline`](../reference/timeline.md) — *the* game's canonical logical-time source

## What it does NOT own

- **No `PhysicsSystem`, `Entity`, or `Collision`.** `Application` has no idea
  these types exist. A game constructs and updates them itself, from inside
  the update callback it hands to `Run`.
- **No networking.** Nothing under `Engine/Network/` is referenced by
  `Application` at all.
- **No game state.** `Application` coordinates *when* things happen (the
  loop), never *what* happens.

## Main public entry point

```cpp
void Run(const UpdateCallback& onUpdate, const RenderCallback& onRender);
```

Constructed once with a `WindowConfig`, then `Run()` is called once — it
blocks for the entire life of the game, calling `onUpdate` then `onRender`
once per frame until the window closes.

## What it coordinates

Each frame, in order: SDL event processing, two built-in debug-key checks
(scaling-mode toggle, Timeline pause/scale controls), a `Timeline` sample,
the game's update callback, and the game's render callback. See the
[Application Loop concept](../concepts/application-loop.md) for the full
mental model and exact order.

## When to interact with it

A game programmer touches `Application` in exactly two places: constructing
it once at startup with a `WindowConfig`, and calling `Run()` with the two
callbacks. Everything else — input queries, drawing, timeline control — goes
through the objects `Run()` hands to those callbacks (`InputManager&`,
`Renderer&`), or through `GetGameTimeline()` if a game needs to read or
change the timeline's scale/pause state itself (see the
[Pause & Slow Motion guide](../guides/pause-and-slow-motion.md)).

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — Application</p>

<dl>
<dt>Purpose</dt>
<dd>Owns the SDL lifecycle and the main update/render loop; the callback seam through which a game participates without living inside the engine.</dd>

<dt>Owner</dt>
<dd>The game's entry point (typically <code>main</code>). Constructed once; documented as the one intended instance, though nothing in code enforces that as a singleton — see <a href="../architecture/application-lifecycle.md">Application Lifecycle</a>.</dd>

<dt>Thread Affinity</dt>
<dd>Single-threaded. <code>Run()</code> blocks the calling thread for the entire game loop; nothing about <code>Application</code> is documented or tested as safe to call from another thread.</dd>

<dt>Blocking Behavior</dt>
<dd><code>Run()</code> blocks until the window is closed or quit is requested. The constructor does not block beyond SDL's own initialization cost.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Neither copyable nor movable — copy is explicitly deleted, which also suppresses the implicit move.</dd>

<dt>Stateful</dt>
<dd>Yes — owns a running flag, and owns <code>Renderer</code>/<code>InputManager</code>/<code>Timeline</code> for its entire lifetime.</dd>

<dt>External Dependencies</dt>
<dd>SDL3 (<code>SDL_Init</code>/<code>SDL_Quit</code>, plus whatever <code>Renderer</code> and <code>InputManager</code> need).</dd>

<dt>Lifetime</dt>
<dd>RAII. Construction calls <code>SDL_Init(SDL_INIT_VIDEO)</code> before creating <code>Renderer</code>/<code>Timeline</code>; destruction explicitly destroys <code>Renderer</code> before calling <code>SDL_Quit()</code> — see <a href="../architecture/application-lifecycle.md">Application Lifecycle</a> for exactly why that order matters.</dd>

<dt>Public Entry Points</dt>
<dd><code>Application(const WindowConfig&)</code>, <code>Run(onUpdate, onRender)</code>, <code>GetGameTimeline()</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>Throws <code>std::runtime_error</code> if <code>SDL_Init</code> fails, or whatever <code>Renderer</code>/<code>Timeline</code> construction throws — either way, SDL is cleanly quit before the exception propagates. No fixed timestep: <code>onUpdate</code>'s <code>deltaTime</code> is whatever real time elapsed since the last frame, uncapped.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Application Loop](../concepts/application-loop.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">The mental model — event processing, input update ordering, timeline sampling, and where your callbacks fit.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: Application](../reference/application.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — constructor, <code>Run</code>, callback types, accessors.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Architecture: Application Lifecycle](../architecture/application-lifecycle.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Construction/destruction order, SDL initialization, and why Renderer must die before <code>SDL_Quit()</code>.</p>
</div>

</div>
