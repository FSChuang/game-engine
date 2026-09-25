# Input

`InputManager` is a thin wrapper over SDL's *polled* keyboard state — it
does not use SDL's event queue at all. It answers exactly two questions:
is this key held right now, and did it just transition from up to down.

## What problem it solves

"Is this key held" and "was this key *just* pressed" are both common
per-frame questions, but they need different information: held is a single
live check, while just-pressed needs to compare against what the key was
doing last frame. `InputManager` tracks that one piece of history —
last frame's state — so a game doesn't have to.

## What it owns

Only a snapshot of the keyboard state as of the last `Update()` call.
Nothing else.

## What it does NOT own

- **No key-binding or action-mapping layer.** Callers check raw
  `SDL_Scancode` values directly (`SDL_SCANCODE_SPACE`, not `"Jump"`).
- **No mouse or gamepad support.** Keyboard only.
- **No event queue of its own.** SDL's event queue (`SDL_PollEvent`) is
  drained by `Application::ProcessEvents` for quit/window-close only;
  `InputManager` never touches it.

## Public entry point

```cpp
void Update();
bool IsKeyPressed(SDL_Scancode key) const;
bool IsKeyJustPressed(SDL_Scancode key) const;
```

## Important semantics

`IsKeyPressed` queries SDL's **live** keyboard state on every call — it is
never a cached snapshot. `IsKeyJustPressed` is `IsKeyPressed(key) &&` the
key was *not* down in the snapshot taken at the last `Update()` call. See
[Input State](../concepts/input-state.md) for the full held-vs-just-pressed
model, including a worked frame-by-frame table.

## Update() ordering matters

`Update()` must be called exactly once per frame, and *when* it's called
determines what "previous state" means for every `IsKeyJustPressed` check
that frame. `Application` calls it **last** — after both the update and
render callbacks — so every just-pressed check made anywhere during a
frame (the built-in debug keys, and your own update callback) compares
against the snapshot taken at the very end of the *previous* frame. See the
[Application Loop concept](../concepts/application-loop.md#input-update-ordering)
for why that ordering was chosen.

## Relationship to Application

`Application` owns exactly one `InputManager`, as a plain (non-`Scope`)
member, for its entire lifetime — see
[Application Lifecycle](../architecture/application-lifecycle.md) for why it
doesn't need the same deferred-construction treatment `Renderer`/`Timeline`
do.

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — InputManager</p>

<dl>
<dt>Purpose</dt>
<dd>Polled keyboard state — held vs. just-pressed — without touching SDL's event queue.</dd>

<dt>Owner</dt>
<dd><code>Application</code> owns exactly one instance, as a plain member, for its entire lifetime.</dd>

<dt>Thread Affinity</dt>
<dd>Single-threaded (main thread), by convention — SDL's own keyboard-state query is meant to be read from the thread that pumps events.</dd>

<dt>Blocking Behavior</dt>
<dd>None — every method is a synchronous, non-blocking query.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Not restricted — no deleted copy/move. Copying duplicates the previous-frame snapshot array; not something real code does, since <code>Application</code> owns the one instance games interact with by reference.</dd>

<dt>Stateful</dt>
<dd>Yes — one previous-frame keyboard-state snapshot.</dd>

<dt>External Dependencies</dt>
<dd>SDL3 (<code>SDL_GetKeyboardState</code>).</dd>

<dt>Public Entry Points</dt>
<dd><code>Update</code>, <code>IsKeyPressed</code>, <code>IsKeyJustPressed</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>Keyboard only — no mouse or gamepad. No key-binding/action layer. Not unit-tested: unlike Timeline/PhysicsSystem/Collision, <code>InputManager</code> reads live SDL state directly rather than through an injectable seam, so it has no existing automated test coverage.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Input State](../concepts/input-state.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Held vs. just-pressed, explained frame by frame.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: InputManager](../reference/input-manager.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — every method, with SDL_Scancode explained.</p>
</div>

</div>
