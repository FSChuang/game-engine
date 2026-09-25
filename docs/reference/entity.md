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

## Synopsis

```cpp
namespace Engine
{
	struct Color
	{
		uint8_t R;
		uint8_t G;
		uint8_t B;
		uint8_t A;
	};

	class Entity
	{
	public:
		Entity(Vector2 position, Vector2 size, Color color);

		Vector2 GetPosition() const;
		void SetPosition(Vector2 position);
		void Move(Vector2 delta);

		Vector2 GetSize() const;
		Color GetColor() const;

		Vector2 GetVelocity() const;
		void SetVelocity(Vector2 velocity);
	};
}
```

## Color

```cpp
struct Color
{
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t A;
};
```

A flat RGBA color, one byte per channel. Set once at construction via
`Entity`'s constructor — there is no setter.

## Entity

```cpp
Entity(Vector2 position, Vector2 size, Color color);
```

Constructs an entity with the given position, size, and color. Velocity
starts at `{0, 0}` regardless of the constructor arguments — there is no way
to set an initial velocity except by calling `SetVelocity` afterward.

## Entity::GetPosition / SetPosition / Move

```cpp
Vector2 GetPosition() const;
void SetPosition(Vector2 position);
void Move(Vector2 delta);
```

`SetPosition` replaces the position outright. `Move` adds `delta` to the
current position (`position.X += delta.X`, and the same for `Y`) — it does
not touch velocity.

## Entity::GetSize / GetColor

```cpp
Vector2 GetSize() const;
Color GetColor() const;
```

Read-only. Both are fixed at construction — there is no `SetSize` or
`SetColor`.

## Entity::GetVelocity / SetVelocity

```cpp
Vector2 GetVelocity() const;
void SetVelocity(Vector2 velocity);
```

Plain storage — `Entity` never reads or changes its own velocity. Only
external code (typically `PhysicsSystem::Update`) does.

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
