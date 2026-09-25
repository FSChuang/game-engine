# Collision

A strict axis-aligned bounding-box (AABB) overlap test — one free function,
no class, no state. That's the entire subsystem.

## What it does

`IsColliding(const Entity& a, const Entity& b)` answers exactly one
question: do these two entities' rectangles overlap right now, using each
entity's position and size. Touching edges or a single shared corner do
**not** count as colliding — the overlap must be strict.

## What it does NOT do

- **No collision response.** `IsColliding` returns a `bool` and nothing
  else — no resolution, no push-apart, no physics interaction. Reacting to
  a collision is entirely the caller's own code.
- **No broad phase.** Every call is a direct, single-pair test with no
  acceleration structure. Checking many entities against each other is the
  caller's own loop.
- **No persistent contact information.** No manifold, no penetration depth,
  no contact normal — just true or false.

<div class="ge-contract" markdown>

<p class="ge-contract__title">System Contract — Collision</p>

<dl>
<dt>Purpose</dt>
<dd>Strict AABB overlap test between two entities.</dd>

<dt>Owner</dt>
<dd>N/A — a pure free function, not a class. Nothing to construct or own.</dd>

<dt>Thread Affinity</dt>
<dd>Safe to call from any thread — it only reads its two <code>const Entity&amp;</code> arguments and touches no shared state.</dd>

<dt>Blocking Behavior</dt>
<dd>None — pure computation.</dd>

<dt>Copy / Move Semantics</dt>
<dd>Not applicable — no instances exist.</dd>

<dt>Stateful</dt>
<dd>No — holds no state between calls.</dd>

<dt>External Dependencies</dt>
<dd>None beyond <code>Engine::Entity</code>/<code>Vector2</code>.</dd>

<dt>Public Entry Points</dt>
<dd><code>IsColliding(const Entity&amp;, const Entity&amp;)</code>.</dd>

<dt>Known Limitations / Failure Modes</dt>
<dd>Pairwise only. No collision response. No broad-phase acceleration structure — every call is an independent O(1) test.</dd>

</dl>

</div>

## Where to go next

See [Reference: Collision](../reference/collision.md) for the exact
overlap formula. No separate concept or guide exists for this subsystem —
there isn't enough here to warrant one.
