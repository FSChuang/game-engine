# Physics

`PhysicsSystem` applies configurable gravity to one `Entity` at a time via
semi-implicit Euler integration. It is a single, small piece of motion math
— nothing about "physics" beyond gravity and velocity exists here.

## What problem it solves

Falling, jumping, and any other gravity-driven motion all boil down to the
same two steps every frame: add gravity to vertical velocity, then move by
velocity. `PhysicsSystem` does that once, correctly, so a game doesn't
hand-roll velocity integration for every object that needs to fall.

## What it owns

Only its own configured gravity value (a single `float`). Nothing else.

## What it does NOT own

- **No entity list.** `Update` takes an `Entity&` argument — `PhysicsSystem`
  holds no reference to any entity between calls, and has no idea how many
  entities exist or which ones a game cares about.
- **No world or scene.** There is no concept of "all physics objects" or a
  simulation you start/stop — each `Update` call is an independent,
  self-contained step for exactly the one entity you pass in.
- **No fixed timestep.** `Update` integrates using whatever `deltaTime` it's
  given, once, however large or small that value is. There is no internal
  accumulator or sub-stepping.

## Public entry point

```cpp
explicit PhysicsSystem(float gravity);

void SetGravity(float gravity);
float GetGravity() const;

void Update(Entity& entity, float deltaTime) const;
```

## Important semantics

`Update` is **semi-implicit Euler**, in this exact order:

1. `velocity.Y += gravity * deltaTime`
2. `entity.SetVelocity(velocity)`
3. `entity.Move({ velocity.X * deltaTime, velocity.Y * deltaTime })`

Position is integrated using the *already-updated* velocity, not the
velocity from before this call — that ordering is what makes it
semi-implicit rather than plain (explicit) Euler. Gravity only ever touches
Y velocity; X velocity passes through `Update` unchanged, however it got
set.

## Caller selects which entities are updated

There is no automatic global list. A game calls `Update(entity, deltaTime)`
once per frame for each entity it wants gravity applied to — entities never
passed to `Update` are simply never touched by `PhysicsSystem` at all.

## Relationship to Collision

`PhysicsSystem::Update` never checks for or resolves overlaps — motion and
collision detection are entirely separate. A game that wants "stop falling
when you hit the ground" calls `IsColliding` itself, after `Update`, and
reacts however it chooses (undo the move, zero the velocity, or anything
else). See [System: Collision](collision.md).

## Relationship to Timeline

`Update` takes a plain `float deltaTime` — `PhysicsSystem` has no idea
`Timeline` exists. The caller is responsible for sourcing that value from
wherever it wants, typically `Application`'s game timeline
(`GetGameTimeline().GetDeltaTime()`), the same way the update callback's own
`deltaTime` parameter is sourced.

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — PhysicsSystem</p>

<dl>
<dt>Purpose</dt>
<dd>Applies configurable gravity to one <code>Entity</code> via semi-implicit Euler integration, on demand.</dd>

<dt>Owner</dt>
<dd>Whichever code constructs it — typically the game layer. One instance can be reused across every entity that shares its gravity value.</dd>

<dt>Thread Affinity</dt>
<dd>No internal synchronization. <code>Update</code> mutates the <code>Entity&amp;</code> passed in, so concurrent calls against the same entity from multiple threads would race.</dd>

<dt>Blocking Behavior</dt>
<dd>None — pure computation.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Not restricted — no deleted copy/move. Copying duplicates the stored gravity value; nothing about an instance is unique or resource-owning.</dd>

<dt>Stateful</dt>
<dd>Yes — one <code>float</code> gravity value, nothing else.</dd>

<dt>External Dependencies</dt>
<dd>None beyond <code>Engine::Entity</code>/<code>Vector2</code> — no SDL.</dd>

<dt>Public Entry Points</dt>
<dd><code>PhysicsSystem(float)</code>, <code>SetGravity</code>/<code>GetGravity</code>, <code>Update(Entity&amp;, float)</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>No collision response of any kind. No fixed or sub-stepped timestep — one <code>Update</code> call integrates exactly once, however large <code>deltaTime</code> is. Gravity affects Y velocity only.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: PhysicsSystem](../reference/physics-system.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API and the integration formula, in order.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Guide: Configure Gravity](../guides/configure-gravity.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Task-oriented: make an entity fall, and pair it with collision.</p>
</div>

</div>
