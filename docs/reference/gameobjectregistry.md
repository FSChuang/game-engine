# GameObjectRegistry

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/GameObject/GameObjectRegistry.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">Core</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">No internal synchronization</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Ownership</span><span class="ge-meta__value">Owned by caller</span></span>
</div>

For the mental model, see [System: GameObjectRegistry](../systems/gameobjectregistry.md).
This page is deliberately just the API facts.

## API Reference

<!-- Generated from Engine/src/Engine/GameObject/GameObjectRegistry.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. -->

--8<-- "gameobjectregistry-api.md"

## Behavior notes

- **`Create`** assigns a fresh, never-reused `ObjectId` (starting at 1) and
  returns a reference that stays valid until that specific object is
  removed — inserting or erasing *other* objects never invalidates it
  (the standard `std::unordered_map` guarantee this class is built on).
- **`FindByNetworkId`/`RemoveByNetworkId`** resolve by a `GameObject`'s
  separate, optional network-identity property, not by `ObjectId` — see
  [GameObject](gameobject.md).

## No SDL dependency

Like `GameObject` itself, `GameObjectRegistry` has no SDL or Network
dependency — it is part of the `EngineCore` CMake target, which a headless
process can link without pulling in SDL.

## Minimal example

```cpp
Engine::GameObjectRegistry objects;

Engine::GameObject& platform =
    objects.Create(Engine::ObjectType::Platform, { 0.0f, 900.0f }, { 1200.0f, 40.0f });
platform.SetRender({ 120, 120, 120, 255 });

Engine::GameObject* found = objects.Find(platform.GetId());
objects.Remove(platform.GetId());
```

## See Also

- [System: GameObjectRegistry](../systems/gameobjectregistry.md)
- [Reference: GameObject](gameobject.md)
