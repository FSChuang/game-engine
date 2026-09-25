# Entity

`Entity` is a generic 2D game object: a position, a size, a color, and a
velocity. Nothing more. It deliberately knows nothing about what kind of
object it represents — player, enemy, wall — so game-specific types can be
layered on top later without this type changing.

## What problem it solves

Renderer, Physics, and Collision all need to operate on *something* with a
position, a size, and (for Physics) a velocity. `Entity` is that shared,
minimal shape — a plain data carrier every other system can read and write
without agreeing on a behavior hierarchy, an ECS, or a component framework
none of them need yet.

## What it owns

Only its own four fields: position, size, color, velocity. Nothing else.

## What it does NOT own

- **No behavior of its own.** There is no `Update()` method, no virtual
  interface, no inheritance hierarchy to override. Whatever happens to an
  `Entity` — gravity, collision checks, drawing — is applied *to* it by
  another system, never *by* it.
- **No identity or registry.** There is no ID, no owning scene, no list of
  "all entities" anywhere in the engine. Nothing tracks which entities exist
  except whatever the game itself stores them in.
- **No relationship to other entities.** No parent/child, no grouping — each
  `Entity` is entirely independent.

## Why other systems operate on it without owning it

`Renderer::DrawEntity`, `PhysicsSystem::Update`, and `IsColliding` all take
an `Entity&` (or `const Entity&`) as a plain function argument — they read
or mutate it for the duration of that one call and store nothing. None of
them hold a reference to any `Entity` between calls, and none of them own
the memory it lives in. Ownership stays entirely with whatever constructed
it — normally the game layer. See
[Entity Model](../concepts/entity-model.md) for the full map of who touches
`Entity` and how.

## Public entry point

```cpp
Entity(Vector2 position, Vector2 size, Color color);

Vector2 GetPosition() const;
void SetPosition(Vector2 position);
void Move(Vector2 delta);

Vector2 GetSize() const;
Color GetColor() const;

Vector2 GetVelocity() const;
void SetVelocity(Vector2 velocity);
```

Size and color are set once at construction — there is no `SetSize` or
`SetColor`. Position and velocity can change at any time.

## When to interact with it

Construct one wherever your game defines its objects, store it however your
game wants (a container, a fixed array, individual named variables — the
engine has no opinion), and hand references to it to `Renderer`,
`PhysicsSystem`, and `IsColliding` as needed.

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — Entity</p>

<dl>
<dt>Purpose</dt>
<dd>A generic 2D game object — position, size, color, velocity — with no behavior of its own.</dd>

<dt>Owner</dt>
<dd>Whichever code constructs it — typically the game layer. The engine holds no registry or list of entities anywhere.</dd>

<dt>Thread Affinity</dt>
<dd>No internal synchronization. Safe only when a single thread accesses a given instance at a time.</dd>

<dt>Blocking Behavior</dt>
<dd>None — every method is a plain accessor/mutator.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Not restricted — no deleted copy/move. Copying duplicates its full state; ordinary value semantics.</dd>

<dt>Stateful</dt>
<dd>Yes — position, size, color, velocity.</dd>

<dt>External Dependencies</dt>
<dd>None — no SDL, nothing beyond Core's <code>Vector2</code>.</dd>

<dt>Public Entry Points</dt>
<dd><code>Entity(Vector2, Vector2, Color)</code>, <code>GetPosition</code>/<code>SetPosition</code>, <code>Move</code>, <code>GetSize</code>, <code>GetColor</code>, <code>GetVelocity</code>/<code>SetVelocity</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>No rotation. No identity/ID. No parent/child or scene-graph relationship. Size and color cannot be changed after construction — there is no setter for either.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: Entity Model](../concepts/entity-model.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Which system reads, writes, or owns Entity — and why none of them own each other.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: Entity](../reference/entity.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — Color, constructor, every accessor.</p>
</div>

</div>
