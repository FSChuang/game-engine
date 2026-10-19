# GameObject Model

`GameObject` (Milestone 3) is compositional, not inheritance-based: every
object has the same identity and Transform, plus three independently
*optional* properties — render color, velocity, and network identity. A
hidden object (a future spawn point or death zone) is simply one that never
called `SetRender`; a static platform is simply one that never called
`SetVelocity`. There is no `Player`/`Platform`/`Enemy` subclass hierarchy —
`GameObject` never becomes a different type, it just has or lacks each
property.

## Who touches GameObject, and how

| System | Reads | Writes | Owns |
|---|---|---|---|
| `Renderer` | `HasRender`, `GetPosition`, `GetSize`, `GetColor` (via `DrawObject`) | — | No |
| `PhysicsSystem` | `HasVelocity` (asserted), `GetVelocity` | `SetVelocity`, position (via `Move`, inside `Update`) | No |
| `IsColliding` | `GetPosition`, `GetSize` | — | No |
| `GameObjectRegistry` | — | Assigns `ObjectId`, stores the object | **Yes, while held** |
| Game / caller | — | Constructs via a registry, calls the above | Decides lifetime |

Every engine-side row reads or writes through a plain reference passed in
for the duration of one call — `DrawObject(const GameObject&)`,
`PhysicsSystem::Update(GameObject&, float)`, `IsColliding(const GameObject&,
const GameObject&)`. None of them store that reference afterward, and none
of them construct a `GameObject` themselves. `Renderer::DrawObject` and
`PhysicsSystem::Update` both treat a missing property as "nothing to do
here" (skip) or "caller error" (assert) rather than inventing a default —
see [System: GameObject](../systems/gameobject.md) for exactly which.

## Ownership stays with the caller

A `GameObjectRegistry` (Milestone 3) is the usual home for a game's
GameObjects — it assigns each one a stable `ObjectId` and owns its storage
for as long as it's held — but nothing *engine-side* reaches into a
registry on its own initiative; Renderer/PhysicsSystem/Collision never hold
one, they only ever receive individual objects a caller already looked up.
A game is free to skip the registry entirely and store a `GameObject` as a
plain member instead (a lone platform, say) — both are ordinary value
types with no required container. See the
[Architecture Overview](../architecture/overview.md) for how this same
"operates on, doesn't own" shape holds across the whole engine, not just
for `GameObject`.

## Network identity is a separate, optional property

A `GameObject`'s network identity (`SetNetworkId`/`GetNetworkId`/
`HasNetworkId`) is a plain `std::uint32_t`, not `Engine::PlayerId` —
`GameObject` has no dependency on the Network module, and the Network
module has no dependency on `GameObject`. A game-side boundary (see
Spare Parts' `NetworkState.h`) is what extracts a `PlayerState` from a
networked `GameObject`, and applies a received `PlayerState` back onto
one — Engine itself never sees the wire format.

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: GameObject](../systems/gameobject.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">What GameObject is, what it owns, and its System Contract.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: GameObject](../reference/gameobject.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — every property, constructor, every accessor.</p>
</div>

</div>
