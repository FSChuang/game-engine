# GameObject

`GameObject` is the engine's compositional runtime game-object model
(Milestone 3): a stable identity (`ObjectId`), a broad category
(`ObjectType` — `Generic`/`Player`/`Platform`/`SpawnPoint`/`DeathZone`/
`Boundary`), a required position/size, and three independently *optional*
properties — render color, velocity, network identity. It deliberately
knows nothing about what a game actually does with it (patrol logic,
respawn rules, P2P publish rates — all game-specific), so game-specific
types never need to layer on top of it as a subclass.

## What problem it solves

Renderer, Physics, and Collision all need to operate on *something* with a
position and a size. `GameObject` is that shared shape — plus, now, a
clean way to say "this particular object also has a color" or "also
moves" or "also has a network identity" without forcing every object to
carry all three. A hidden spawn point/death zone is a `GameObject` with no
render property; a static platform is one with no velocity property.

## What it owns

Its identity/type, its Transform (position + size), and whichever of the
three optional properties a caller has set via `SetRender`/`SetVelocity`/
`SetNetworkId`. Nothing else.

## What it does NOT own

- **No behavior of its own.** There is no `Update()` method, no virtual
  interface, no inheritance hierarchy to override. Whatever happens to a
  `GameObject` — gravity, collision checks, drawing — is applied *to* it by
  another system, never *by* it.
- **No registry of its own.** A `GameObject` does not know which
  `GameObjectRegistry` (if any) holds it, and there is no global list of
  "every GameObject that exists" anywhere in the engine.
- **No relationship to other objects.** No parent/child, no grouping — each
  `GameObject` is entirely independent.

## Why other systems operate on it without owning it

`Renderer::DrawObject`, `PhysicsSystem::Update`, and `IsColliding` all take
a `GameObject&` (or `const GameObject&`) as a plain function argument —
they read or mutate it for the duration of that one call and store
nothing. `DrawObject` silently returns if `HasRender()` is false (a hidden
object needs no color to exist); `PhysicsSystem::Update` asserts if
`HasVelocity()` is false (the caller decided to pass a velocity-less
object, which is a caller error, not a recoverable case). See
[GameObject Model](../concepts/gameobject-model.md) for the full map of
who touches `GameObject` and how.

## Public entry point

```cpp
GameObject(ObjectId id, ObjectType type, Vector2 position, Vector2 size);

ObjectId GetId() const;
ObjectType GetType() const;

Vector2 GetPosition() const;
void SetPosition(Vector2 position);
void Move(Vector2 delta);
Vector2 GetSize() const;

bool HasRender() const;
Color GetColor() const;        // requires HasRender()
void SetRender(Color color);   // attaches the property if absent

bool HasVelocity() const;
Vector2 GetVelocity() const;       // requires HasVelocity()
void SetVelocity(Vector2 velocity); // attaches the property if absent

bool HasNetworkId() const;
std::uint32_t GetNetworkId() const;       // requires HasNetworkId()
void SetNetworkId(std::uint32_t networkId); // attaches the property if absent
```

Size is fixed at construction — there is no `SetSize`. Position and every
optional property can change at any time; `Set*` attaches a property on its
first call and simply replaces its value on every call after that.

## When to interact with it

Most games construct GameObjects through a `GameObjectRegistry` (see
[System: GameObjectRegistry](gameobjectregistry.md)), which assigns each
one a stable `ObjectId`. A single long-lived object with no need for
registry-style lookup (e.g. a lone shared platform) can just as validly be
constructed directly and stored as a plain member — the engine has no
opinion either way.

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — GameObject</p>

<dl>
<dt>Purpose</dt>
<dd>A compositional 2D game object — identity, type, Transform, plus optional render/velocity/network properties — with no behavior of its own.</dd>

<dt>Owner</dt>
<dd>Whichever code constructs it — typically a GameObjectRegistry the game layer holds. The engine itself holds no registry anywhere.</dd>

<dt>Thread Affinity</dt>
<dd>No internal synchronization. Safe only when a single thread accesses a given instance at a time.</dd>

<dt>Blocking Behavior</dt>
<dd>None — every method is a plain accessor/mutator.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Not restricted — no deleted copy/move. Rule of Zero: every member is a plain value, so copying duplicates full state with no custom special members.</dd>

<dt>Stateful</dt>
<dd>Yes — identity, type, Transform, and whichever optional properties have been attached.</dd>

<dt>External Dependencies</dt>
<dd>None — no SDL, no Network module, nothing beyond Core's <code>Vector2</code>.</dd>

<dt>Public Entry Points</dt>
<dd><code>GameObject(ObjectId, ObjectType, Vector2, Vector2)</code>, <code>GetId</code>/<code>GetType</code>, <code>GetPosition</code>/<code>SetPosition</code>, <code>Move</code>, <code>GetSize</code>, <code>HasRender</code>/<code>GetColor</code>/<code>SetRender</code>, <code>HasVelocity</code>/<code>GetVelocity</code>/<code>SetVelocity</code>, <code>HasNetworkId</code>/<code>GetNetworkId</code>/<code>SetNetworkId</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>No rotation. No parent/child or scene-graph relationship. Size cannot change after construction. Calling a property's getter before its matching <code>Set*</code> aborts the process (<code>ENGINE_ASSERT</code>) — always check <code>Has*</code> first if presence isn't already guaranteed.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Concept: GameObject Model](../concepts/gameobject-model.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Which system reads, writes, or owns GameObject — and why none of them own each other.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: GameObject](../reference/gameobject.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — every property, constructor, every accessor.</p>
</div>

</div>
