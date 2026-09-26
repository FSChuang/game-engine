# Application

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Core/Application.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">Core</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">Single-threaded</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Ownership</span><span class="ge-meta__value">Owns Renderer, InputManager, Timeline</span></span>
</div>

For the mental model and lifecycle rationale, see the
[Application Loop concept](../concepts/application-loop.md) and
[Application Lifecycle architecture](../architecture/application-lifecycle.md)
page — this page is deliberately just the API facts.

## API Reference

<!-- Generated from Engine/src/Engine/Core/Application.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. -->

--8<-- "application-api.md"

## Behavior notes

The constructor calls `SDL_Init(SDL_INIT_VIDEO)` first — throws
`std::runtime_error` if that fails, and cleanly calls `SDL_Quit()` before
rethrowing if anything afterward (`Renderer` or `Timeline` construction)
throws. Neither copyable nor movable.

`Run` **blocks the calling thread** until the window closes or quit is
requested. Calls `onUpdate` then `onRender` once per frame, in that order,
with a `Timeline`-derived `deltaTime` sampled fresh each frame, uncapped and
variable (not fixed-step) — see the
[Application Loop concept](../concepts/application-loop.md#the-actual-order-exactly)
for the exact per-frame order this fits into.

`GetGameTimeline` returns the same `Timeline` instance for the life of the
`Application` — use it to read or change scale/pause from your own code; see
the [Pause & Slow Motion guide](../guides/pause-and-slow-motion.md).

`UpdateCallback`/`RenderCallback` are plain `std::function` aliases — any
callable with a matching signature works, not just lambdas.

## Ownership and resource notes

`Application` owns `Renderer` and `Timeline` via `Scope<T>` (`std::unique_ptr`)
and `InputManager` as a plain member. It owns nothing your game defines —
`PhysicsSystem`, `Entity`, `Collision`, and any networking types are never
referenced by `Application` at all.

## Minimal example

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

## See Also

- [System: Application](../systems/application.md)
- [Concept: Application Loop](../concepts/application-loop.md)
- [Architecture: Application Lifecycle](../architecture/application-lifecycle.md)
- [Reference: Timeline](timeline.md)
