# GameObjectRegistry

`GameObjectRegistry` (Milestone 3) owns a set of `GameObject`s, keyed by the
stable `ObjectId` it assigns each one at creation. It is deliberately just a
thin wrapper over `std::unordered_map<ObjectId, GameObject>` — not a scene
graph, not an ECS, no query/systems framework. A game's render/update loop,
or a headless server's connection bookkeeping, can hold one and iterate it
directly.

## What problem it solves

Something needs to own the `GameObject`s a game or server creates, hand out
stable ids for them, and let other code look one up again later — without
inventing an entity-component-system. `GameObjectRegistry` is that, and
nothing more.

## What it owns

Every `GameObject` constructed through it, for as long as it isn't removed.

## Identity lookups

`Find(ObjectId)` resolves the registry's own id. `FindByNetworkId`/
`RemoveByNetworkId` resolve by a `GameObject`'s separate, optional network
identity (see [GameObject](gameobject.md)) instead — both ends of the
network boundary need to answer "which GameObject is remote/connected
player X" without maintaining their own id-translation table.

## Public entry point

```cpp
GameObject& Create(ObjectType type, Vector2 position, Vector2 size);

bool Remove(ObjectId id);
bool RemoveByNetworkId(std::uint32_t networkId);

GameObject* Find(ObjectId id);
GameObject* FindByNetworkId(std::uint32_t networkId);

std::size_t Count() const;
// begin()/end() (and const overloads) iterate every live object.
```

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — GameObjectRegistry</p>

<dl>
<dt>Purpose</dt>
<dd>Owns a set of GameObjects, keyed by a stable ObjectId it assigns at creation.</dd>

<dt>Owner</dt>
<dd>Whichever code constructs it — a game's own update loop, or a headless server's connection bookkeeping.</dd>

<dt>Thread Affinity</dt>
<dd>No internal synchronization. Safe only when a single thread accesses a given instance at a time.</dd>

<dt>Blocking Behavior</dt>
<dd>None.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Not restricted — ordinary value semantics over its underlying map.</dd>

<dt>External Dependencies</dt>
<dd>None beyond <code>Engine::GameObject</code> — no SDL, no Network module.</dd>

<dt>Public Entry Points</dt>
<dd><code>Create</code>, <code>Remove</code>/<code>RemoveByNetworkId</code>, <code>Find</code>/<code>FindByNetworkId</code>, <code>Count</code>, <code>begin</code>/<code>end</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd><code>FindByNetworkId</code>/<code>RemoveByNetworkId</code> are a linear scan — fine at the small (roughly MaxPlayers-scale) object counts this engine targets, not a large-scene lookup structure.</dd>

</dl>

</div>

## Where to go next

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[System: GameObject](gameobject.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">The type this registry stores.</p>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Reference: GameObjectRegistry](../reference/gameobjectregistry.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Exact API facts.</p>
</div>

</div>
