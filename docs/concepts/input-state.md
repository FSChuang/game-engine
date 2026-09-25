# Input State

"Held" and "just pressed" both come from comparing two things: what a key is
doing right now, and what it was doing last frame. `InputManager` tracks
only the second half — the first is always read live from SDL.

## The two pieces of state

- **Current** — always read live from SDL, at the exact moment
  `IsKeyPressed`/`IsKeyJustPressed` is called. Never cached.
- **Previous** — a snapshot taken by the last `Update()` call, held until
  the next `Update()` replaces it.

## Held vs. just pressed

- **Held** (`IsKeyPressed`): `current == down`. True for as long as the key
  stays down, including the very first frame.
- **Just pressed** (`IsKeyJustPressed`): `current == down && previous ==
  up`. True for exactly one frame — the one where the key transitioned —
  and false on every frame after, even while still held.

## Worked example

A key pressed on frame 2 and released on frame 4, assuming `Update()` runs
once at the end of every frame:

| Frame | Current (live) | Previous (snapshot) | Held | Just pressed |
|---|---|---|---|---|
| 1 | up | up | false | false |
| 2 | **down** | up | **true** | **true** |
| 3 | down | down | true | false |
| 4 | up | down | false | false |

Frame 3 is the important row: the key is still held, but it's no longer
*just* pressed — `IsKeyJustPressed` only fires on the one transition frame.

## No event buffering

`InputManager` does not queue key-down events. If a key is checked, that
check reads whatever SDL reports as the live state at that instant — there
is no history beyond the single previous-frame snapshot, and no guarantee
that a very short press between two `Update()` calls is ever observed.

## Why Update() runs where it does

`Update()`'s snapshot only makes sense relative to *when* it's taken.
`Application` calls it last in the frame — after both the update and render
callbacks — so every `IsKeyJustPressed` check made anywhere during a frame
compares against the state from the end of the *previous* frame, consistently.
See the
[Application Loop concept](application-loop.md#input-update-ordering) for
the exact ordering this fits into.

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: Input](../systems/input.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">What InputManager owns, and its System Contract.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: InputManager](../reference/input-manager.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API and SDL_Scancode.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Application Loop](application-loop.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">The full per-frame order Update() fits into.</p>
</div>

</div>
