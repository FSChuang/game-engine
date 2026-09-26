# Entity

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Entity/Entity.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">Core</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">No internal synchronization</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Ownership</span><span class="ge-meta__value">Owned by caller — no engine-side registry</span></span>
</div>

For the mental model, see [Entity Model](../concepts/entity-model.md). This
page is deliberately just the API facts.

## API Reference

<!-- Generated from Engine/src/Engine/Entity/Entity.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. -->

--8<-- "entity-api.md"

## Behavior notes

- **`Color`** is set once at construction via `Entity`'s constructor — there
  is no setter.
- **`Entity`'s constructor** never sets an initial velocity — velocity starts
  at `{0, 0}` regardless of the constructor arguments; the only way to set
  one is calling `SetVelocity` afterward.
- **`GetVelocity`/`SetVelocity`** are plain storage — `Entity` never reads or
  changes its own velocity. Only external code (typically
  `PhysicsSystem::Update`) does.

## Copy / move / value semantics

No copy or move operations are declared or deleted — `Entity` has ordinary
value semantics. Copying one duplicates its full state (position, size,
color, velocity).

## No SDL dependency

`Entity` includes only `Engine/Core/Core.h` for `Vector2` — nothing about it
requires SDL, a window, or a renderer to exist.

## Minimal example

```cpp
Engine::Entity player(
    { 100.0f, 100.0f },  // position
    { 32.0f, 32.0f },    // size
    { 255, 0, 0, 255 }); // color: opaque red

player.SetVelocity({ 0.0f, -50.0f });
player.Move({ 5.0f, 0.0f });
```

## See Also

- [System: Entity](../systems/entity.md)
- [Concept: Entity Model](../concepts/entity-model.md)
- [Reference: Renderer](renderer.md)
- [Reference: PhysicsSystem](physics-system.md)
- [Reference: Collision](collision.md)
