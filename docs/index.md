<!-- Homepage title lives in the <title> via mkdocs' nav entry ("Home"); this
     H1 is the on-page hero heading, kept small and factual on purpose. -->
# Game Engine

A lightweight C++17 / SDL3 engine built around explicit ownership,
deterministic timing, and simple multiplayer networking.

[Get started](getting-started/installation.md){ .md-button .md-button--primary }
[Architecture overview](architecture/overview.md){ .md-button }

## Core principles

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Explicit ownership</p>
<p class="ge-card__purpose">Every resource has one owner, expressed in the type — <code>Scope&lt;T&gt;</code>, <code>Ref&lt;T&gt;</code>, or a class that deletes copy when it must.</p>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">RAII resource management</p>
<p class="ge-card__purpose">SDL handles, ZeroMQ sockets — owned by a wrapper whose destructor releases them, on every path.</p>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Small, reusable systems</p>
<p class="ge-card__purpose">Renderer, Physics, Collision, Input, Time each solve one problem and know nothing about the others.</p>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Engine / game separation</p>
<p class="ge-card__purpose">Reusable, game-agnostic code lives in the engine. Anything specific to one game never leaks in.</p>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Deterministic logical time</p>
<p class="ge-card__purpose"><code>Timeline</code> separates logical time from wall-clock time — scale, pause, and tic size are explicit, never hidden.</p>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Thread-affine networking primitives</p>
<p class="ge-card__purpose">A <code>Socket</code> is built, used, and destroyed on exactly one thread — a documented contract, not an accident.</p>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Minimal abstraction</p>
<p class="ge-card__purpose">No abstraction is added for a use case that doesn't exist yet. Simplicity is the default.</p>
</div>

</div>

## Engine systems

<div class="ge-card-grid" markdown>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Application](systems/application.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Owns the SDL lifecycle and the main update/render loop.</p>
<span class="ge-card__tag">Core</span>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Rendering</p>
<p class="ge-card__purpose">Window, drawing, and constant/proportional scaling modes.</p>
<span class="ge-card__tag">SDL</span>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Entity</p>
<p class="ge-card__purpose">A generic position + size + color + velocity object — no behavior.</p>
<span class="ge-card__tag">Core</span>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Physics</p>
<p class="ge-card__purpose">Configurable-gravity, semi-implicit Euler integration.</p>
<span class="ge-card__tag">Core</span>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Input</p>
<p class="ge-card__purpose">Polled keyboard state: held vs. just-pressed.</p>
<span class="ge-card__tag">SDL</span>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Collision</p>
<p class="ge-card__purpose">Strict axis-aligned bounding-box overlap.</p>
<span class="ge-card__tag">Core</span>
</div>

<div class="ge-card ge-card--linked" markdown>
<p class="ge-card__title" markdown>[Time](systems/time.md){: .ge-card__link } <span class="ge-card__arrow">→</span></p>
<p class="ge-card__purpose">Logical time: scale, pause, and tic size, independent of the wall clock.</p>
<span class="ge-card__tag">Core</span>
</div>

<div class="ge-card ge-card--static" markdown>
<p class="ge-card__title">Networking</p>
<p class="ge-card__purpose">Wire protocol, pure dispatch logic, and a thread-affine ZeroMQ socket wrapper.</p>
<span class="ge-card__tag">Optional — ZeroMQ</span>
</div>

</div>

*Application and Time now have full system pages — the rest are coming in a
later documentation pass; see the [Architecture Overview](architecture/overview.md)
for how they all fit together today.*

## Architecture, briefly

The engine is built as its own static library, so every game that links it
shares the same reusable core — the engine/game boundary is enforced by the
build graph, not just convention. Read the full
[Architecture Overview](architecture/overview.md) for the ownership model,
dependency tiers, and where networking fits.

## Quick start

```cpp
#include "Engine/Core/Application.h"

int main()
{
	Engine::WindowConfig windowConfig{ "My Game", 1920, 1080 };
	Engine::Application application(windowConfig);

	application.Run(
	    [](Engine::InputManager& input, float deltaTime)
	    {
		    // Update game state here.
	    },
	    [](Engine::Renderer& renderer)
	    {
		    // Draw entities here.
	    });

	return 0;
}
```

`Application::Run` blocks until the window is closed, calling your update
callback then your render callback once per frame.

## Where to go next

- **New to the engine?** Start with [Installation](getting-started/installation.md), then [Build & Run](getting-started/build-and-run.md).
- **Want the big picture?** Read the [Architecture Overview](architecture/overview.md).
