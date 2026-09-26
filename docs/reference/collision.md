# Collision

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Collision/Collision.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">Core</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">Safe from any thread (reads only)</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Blocking</span><span class="ge-meta__value">Never</span></span>
</div>

For the mental model, see [System: Collision](../systems/collision.md). This
page is deliberately just the API facts.

## API Reference

<!-- Generated from Engine/src/Engine/Collision/Collision.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. -->

--8<-- "collision-api.md"

## Behavior notes

**Parameters** — `a`, `b`: the two entities to test, read via their
`GetPosition()`/`GetSize()`.

**Returns** — `true` if the two entities' axis-aligned rectangles overlap by
more than zero area on both axes; `false` otherwise.

**Exact semantics** — on each axis, the two rectangles are considered
*separated* if one's trailing edge is at or before the other's leading edge
(`a.X + a.Width <= b.X`, or the symmetric case). Because this uses `<=`
rather than `<`, two rectangles that exactly touch — sharing an edge or
only a single corner — are already separated on at least one axis, so
`IsColliding` returns `false`. Overlap must be strict on **both** axes for
the result to be `true`.

## Minimal example

```cpp
Engine::Entity a({ 0.0f, 0.0f }, { 10.0f, 10.0f }, { 255, 255, 255, 255 });
Engine::Entity b({ 5.0f, 5.0f }, { 10.0f, 10.0f }, { 255, 255, 255, 255 });
bool overlapping = Engine::IsColliding(a, b); // true

Engine::Entity c({ 10.0f, 0.0f }, { 10.0f, 10.0f }, { 255, 255, 255, 255 });
bool touchingOnly = Engine::IsColliding(a, c); // false — edges only touch
```

## See Also

- [System: Collision](../systems/collision.md)
- [Reference: Entity](entity.md)
- [System: Physics](../systems/physics.md)
