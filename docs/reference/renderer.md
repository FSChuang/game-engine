# Renderer

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Renderer/Renderer.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">SDL</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">Single-threaded</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Ownership</span><span class="ge-meta__value">Owns SDL_Window / SDL_Renderer</span></span>
</div>

For the mental model, see [Rendering & Scaling](../concepts/rendering-and-scaling.md).
This page is deliberately just the API facts.

## WindowConfig, at a glance

`Width`/`Height` set the initial SDL window size **and** become the fixed
*reference resolution* that `Proportional` scaling measures against for the
renderer's entire lifetime — there is no way to change the reference
resolution after construction.

## API Reference

<!-- Generated from Engine/src/Engine/Renderer/Renderer.h by
     scripts/generate_api_docs.py (Documentation Phase 4 pilot) — do not
     hand-edit the section below; edit the header's /// comments instead and
     regenerate. -->

--8<-- "renderer-api.md"

## Behavior notes

- **Scaling math** — see [Rendering & Scaling](../concepts/rendering-and-scaling.md)
  for the exact formula and worked examples. `ApplyScalingMode` is applied
  independently to both position and size by `DrawEntity`, and does not read
  or modify any `Entity`/`Renderer` state itself.
- **Construction** creates the window and renderer together via
  `SDL_CreateWindowAndRenderer`; if creation fails, whichever handle *did*
  succeed is destroyed before the exception is thrown — construction never
  leaves a partial resource behind.
- **Frame order** — call `BeginFrame`, then any number of `DrawEntity` calls,
  then `EndFrame` — exactly the order `Application::Run` already uses around
  your render callback.
- **`DrawEntity` reads live state** — the window's *current* size is read
  fresh on every call (`SDL_GetWindowSize`), never cached, so a scaling-mode
  change takes effect on the very next `DrawEntity` call; nothing already
  drawn is retroactively affected.

## Ownership and resource notes

`Renderer` owns its `SDL_Window*`/`SDL_Renderer*` for its entire lifetime and
exposes neither to callers. It holds no reference to any `Entity` — every
`DrawEntity` call reads its argument once and stores nothing.

## Minimal example

```cpp
application.Run(
    [](Engine::InputManager& input, float deltaTime)
    {
        // Update game state here.
    },
    [](Engine::Renderer& renderer)
    {
        renderer.DrawEntity(player);
        renderer.DrawEntity(wall);
    });
```

## See Also

- [System: Renderer](../systems/renderer.md)
- [Concept: Rendering & Scaling](../concepts/rendering-and-scaling.md)
- [Guide: Toggle Scaling Mode](../guides/toggle-scaling-mode.md)
- [Reference: Entity](entity.md)
