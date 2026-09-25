# Entity Model

`Entity` itself is just four fields. What makes it work as the engine's
shared game-object type is that `Renderer`, `PhysicsSystem`, and `IsColliding`
all agree to operate *on* it through plain function arguments, without any
of them owning it — or each other.

## Who touches Entity, and how

| System | Reads | Writes | Owns |
|---|---|---|---|
| `Renderer` | `GetPosition`, `GetSize`, `GetColor` (via `DrawEntity`) | — | No |
| `PhysicsSystem` | `GetVelocity` | `SetVelocity`, position (via `Move`, inside `Update`) | No |
| `IsColliding` | `GetPosition`, `GetSize` | — | No |
| Game / caller | — | Constructs it, stores it, calls the above | **Yes** |

Every engine-side row reads or writes through a plain reference passed in
for the duration of one call — `DrawEntity(const Entity&)`,
`PhysicsSystem::Update(Entity&, float)`, `IsColliding(const Entity&, const
Entity&)`. None of them store that reference afterward, and none of them
construct an `Entity` themselves.

## Ownership stays with the caller

Because nothing engine-side owns an `Entity`, there is no scene graph, no
"entity list" living inside the engine, and no lifetime the engine manages
for you. Whatever your game uses to store its entities — a `std::vector`, a
fixed array, individual named objects — is the only place that ownership
exists. See the [Architecture Overview](../architecture/overview.md) for how
this same "operates on, doesn't own" shape holds across the whole engine,
not just for `Entity`.

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: Entity](../systems/entity.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">What Entity is, what it owns, and its System Contract.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: Entity](../reference/entity.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API — Color, constructor, every accessor.</p>
</div>

</div>
