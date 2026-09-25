# PhysicsSystem

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Physics/PhysicsSystem.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">Core</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">No internal synchronization</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Blocking</span><span class="ge-meta__value">Never</span></span>
</div>

For the mental model, see [System: Physics](../systems/physics.md). This
page is deliberately just the API facts.

## Synopsis

```cpp
namespace Engine
{
	class PhysicsSystem
	{
	public:
		explicit PhysicsSystem(float gravity);

		void SetGravity(float gravity);
		float GetGravity() const;

		void Update(Entity& entity, float deltaTime) const;
	};
}
```

## PhysicsSystem

```cpp
explicit PhysicsSystem(float gravity);
```

Constructs a physics system configured with the given gravity value. There
is no default constructor — a gravity value is always required.

## PhysicsSystem::SetGravity / GetGravity

```cpp
void SetGravity(float gravity);
float GetGravity() const;
```

Plain storage — no validation, no clamping. Any `float` is accepted,
including zero or negative (upward) gravity.

## PhysicsSystem::Update

```cpp
void Update(Entity& entity, float deltaTime) const;
```

Applies one semi-implicit Euler integration step, in this exact order:

```
velocity.Y += gravity * deltaTime
entity.SetVelocity(velocity)
entity.Move({ velocity.X * deltaTime, velocity.Y * deltaTime })
```

Position is integrated using the **already-updated** velocity — not the
velocity `entity` had before this call. `deltaTime` is in the same units as
`Timeline::GetDeltaTime()` (seconds); `Update` performs no unit conversion
of its own. A `deltaTime` of `0` produces no change to either velocity or
position.

`entity` is mutated directly through its own `SetVelocity`/`Move` — `Update`
holds no state about `entity` between calls; every call reads whatever
velocity `entity` currently reports and computes from that.

## Example

```cpp
Engine::PhysicsSystem physics(10.0f); // gravity = 10 units/s²

Engine::Entity ball({ 0.0f, 0.0f }, { 10.0f, 10.0f }, { 255, 255, 255, 255 });
physics.Update(ball, 0.5f);
// ball.GetVelocity().Y == 5.0f   (0 + 10 * 0.5)
// ball.GetPosition().Y == 2.5f  (0 + 5.0 * 0.5, using the updated velocity)
```

## See Also

- [System: Physics](../systems/physics.md)
- [Guide: Configure Gravity](../guides/configure-gravity.md)
- [Reference: Entity](entity.md)
- [System: Collision](../systems/collision.md)
