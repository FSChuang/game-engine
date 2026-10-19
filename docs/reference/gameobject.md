# GameObject

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/GameObject/GameObject.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">Core</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">No internal synchronization</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Ownership</span><span class="ge-meta__value">Owned by caller — typically a GameObjectRegistry</span></span>
</div>

For the mental model, see [GameObject Model](../concepts/gameobject-model.md). This
page is deliberately just the API facts.

## API Reference

<!-- Generated from Engine/src/Engine/GameObject/GameObject.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. -->

--8<-- "gameobject-api.md"

## Behavior notes

- **`ObjectId`/`ObjectType`/position/size** are fixed or assigned at
  construction; only `ObjectId`/`ObjectType`/size have no setter at all —
  position can change via `SetPosition`/`Move`.
- **Render, velocity, and network-id are independently optional.** Each
  starts absent (`Has*() == false`). The first `Set*` call attaches the
  property; every call after that just replaces its value.
- **`GetColor`/`GetVelocity`/`GetNetworkId`** each assert their matching
  `Has*()` is true (`ENGINE_ASSERT`, which aborts in debug builds and is
  compiled out in release) — a caller must check presence first unless it's
  already guaranteed by construction.
- **Network identity is a plain `std::uint32_t`**, not `Engine::PlayerId` —
  `GameObject` has no dependency on the Network module in either direction.

## Copy / move / value semantics

No copy or move operations are declared or deleted — `GameObject` has
ordinary value semantics (Rule of Zero: every member is a plain value —
`ObjectId`, `ObjectType`, two `Vector2`s, three small `std::optional`s).
Copying one duplicates its full state.

## No SDL dependency

`GameObject` includes only `Engine/Core/Core.h` for `Vector2` — nothing
about it requires SDL, a window, a renderer, or the Network module to
exist. This is why a headless process (e.g. Spare Parts' dedicated
multiplayer server) can use `GameObject`/`GameObjectRegistry` without
linking SDL at all — see `EngineCore` in
[Project Structure](../../PROJECT_STRUCTURE.md).

## Minimal example

```cpp
Engine::GameObject player(
    1,                                  // id
    Engine::ObjectType::Player,
    { 100.0f, 100.0f },                 // position
    { 32.0f, 32.0f });                  // size

player.SetRender({ 255, 0, 0, 255 });   // opaque red
player.SetVelocity({ 0.0f, -50.0f });
player.Move({ 5.0f, 0.0f });

// A hidden object needs none of the above:
Engine::GameObject spawnPoint(2, Engine::ObjectType::SpawnPoint, { 400.0f, 0.0f }, { 1.0f, 1.0f });
assert(!spawnPoint.HasRender());
```

## See Also

- [System: GameObject](../systems/gameobject.md)
- [System: GameObjectRegistry](../systems/gameobjectregistry.md)
- [Concept: GameObject Model](../concepts/gameobject-model.md)
- [Reference: GameObjectRegistry](gameobjectregistry.md)
- [Reference: Renderer](renderer.md)
- [Reference: PhysicsSystem](physics-system.md)
- [Reference: Collision](collision.md)
