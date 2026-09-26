# Configure Gravity

## Goal

Make an entity accelerate downward (or upward, or not at all) using
`PhysicsSystem`.

## Prerequisites

- An `Entity` to update.
- A per-frame `deltaTime` source — typically
  `application.GetGameTimeline().GetDeltaTime()`, or the `deltaTime` your
  update callback already receives.

## Minimal example

```cpp
Engine::PhysicsSystem physics(980.0f); // gravity, in your world's units/s²

// Once per frame, for each entity gravity should affect:
physics.Update(player, deltaTime);
```

## Step-by-step

1. **Construct one `PhysicsSystem` with a gravity value.** A single instance
   can be reused across every entity that shares the same gravity — there is
   no per-entity state inside `PhysicsSystem` itself.
2. **Call `Update(entity, deltaTime)` once per frame, per entity.** There is
   no automatic list — an entity you never pass to `Update` is never
   touched by gravity at all.
3. **Change gravity later with `SetGravity`, if needed.** A power-up, a
   low-gravity zone, or a pause can all just call `SetGravity` with a new
   value.
4. **Handle collisions yourself, separately.** `Update` never checks for or
   resolves overlaps — see [System: Collision](../systems/collision.md) for
   the strict overlap test to pair this with.

## Why it works

`Update` adds `gravity * deltaTime` to the entity's Y velocity, then moves
the entity by the *new* velocity times `deltaTime` — semi-implicit Euler.
See [Reference: PhysicsSystem](../reference/physics-system.md) for the exact
formula and order.

!!! warning "This is not a physics world"
    `PhysicsSystem` has no concept of rigid bodies, constraints, joints, or
    broad-phase collision detection. It does exactly one thing — gravity via
    velocity integration — for exactly the entity you pass to `Update`.
    Combining falling entities with collision response is entirely your own
    code's responsibility.

## Common mistakes

- **Forgetting to call `Update` every frame.** Gravity never accumulates on
  its own — nothing runs automatically.
- **Assuming `Update` resolves collisions.** It does not. See
  [System: Collision](../systems/collision.md).
- **Passing a `Timeline` directly.** `Update` takes a plain `float` — extract
  the value yourself first, e.g. via
  `application.GetGameTimeline().GetDeltaTime()`.

## Related APIs

- [`PhysicsSystem::Update`](../reference/physics-system.md#physicssystemupdate)
- [`PhysicsSystem::SetGravity` / `GetGravity`](../reference/physics-system.md#physicssystemsetgravity)

## Next steps

- Read [Reference: PhysicsSystem](../reference/physics-system.md) for the
  full API.
- Read [System: Collision](../systems/collision.md) to pair falling entities
  with overlap detection.
