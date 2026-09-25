# Rendering & Scaling

`Renderer` draws every entity through the same scaling step, so a game can
switch — at runtime — between two ways of mapping an entity's coordinates
onto the window: track the window size, or ignore it entirely.

## The two modes

- **Constant** — the entity's position and size are used exactly as given,
  in pixel units. Resizing the window changes nothing about where or how
  big entities are drawn.
- **Proportional** — the entity's position and size are scaled by
  `currentWindowSize / referenceResolution`, independently on each axis.
  The reference resolution is fixed forever at construction (the
  `WindowConfig` width/height passed to `Renderer`); only the *current*
  window size changes as the window is resized.

## The exact math

```
Constant:      result = value
Proportional:  result.X = value.X * (currentResolution.X / referenceResolution.X)
               result.Y = value.Y * (currentResolution.Y / referenceResolution.Y)
```

`Renderer::DrawEntity` applies this **twice** per entity, independently — once
to its position, once to its size — using whatever the window's current size
actually is at the moment of that draw call.

## Worked examples

These mirror the engine's own scaling tests exactly.

| Reference resolution | Current resolution | Input value | Constant result | Proportional result |
|---|---|---|---|---|
| 1920 × 1080 | 960 × 540 (halved) | (100, 200) | (100, 200) — unchanged | (50, 100) — halved on both axes |
| 1000 × 1000 | 500 × 2000 (non-uniform) | (10, 10) | (10, 10) — unchanged | (5, 20) — **X and Y scaled independently** |

The second row is the important one: because X and Y scale independently,
resizing to a *different aspect ratio* than the reference resolution
visibly stretches or squashes every `Proportional`-scaled entity. Nothing in
`Renderer` corrects for this — there is no letterboxing or aspect-ratio
preservation.

## Why size is scaled too, not just position

Scaling only position while leaving size untouched would make entities
correctly *placed* but the wrong shape relative to the window. `DrawEntity`
scales both with the same call, so an entity's proportions relative to the
window stay consistent under `Proportional` mode.

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: Renderer](../systems/renderer.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">What Renderer owns, what it doesn't, and its System Contract.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: Renderer](../reference/renderer.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — WindowConfig, ScalingMode, ApplyScalingMode.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Guide: Toggle Scaling Mode](../guides/toggle-scaling-mode.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Switch modes from your own code, not just the built-in Tab key.</p>
</div>

</div>
