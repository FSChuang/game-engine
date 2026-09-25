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

## Synopsis

```cpp
namespace Engine
{
	class Application
	{
	public:
		using UpdateCallback = std::function<void(InputManager& input, float deltaTime)>;
		using RenderCallback = std::function<void(Renderer& renderer)>;

		explicit Application(const WindowConfig& windowConfig);
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		void Run(const UpdateCallback& onUpdate, const RenderCallback& onRender);

		Timeline& GetGameTimeline();
	};
}
```

## `Application(const WindowConfig&)`

Constructs the window/renderer, input manager, and game timeline. Calls
`SDL_Init(SDL_INIT_VIDEO)` first — throws `std::runtime_error` if that fails,
and cleanly calls `SDL_Quit()` before rethrowing if anything afterward
(`Renderer` or `Timeline` construction) throws.

Neither copyable nor movable.

## `Application::Run`

```cpp
void Run(const UpdateCallback& onUpdate, const RenderCallback& onRender);
```

The main loop. **Blocks the calling thread** until the window closes or quit
is requested. Calls `onUpdate` then `onRender` once per frame, in that order,
with a `Timeline`-derived `deltaTime` sampled fresh each frame — see the
[Application Loop concept](../concepts/application-loop.md#the-actual-order-exactly)
for the exact per-frame order this fits into.

`deltaTime` is variable, not fixed-step — whatever real time elapsed since
the last frame, subject to the game timeline's current scale/pause.

## `Application::GetGameTimeline`

```cpp
Timeline& GetGameTimeline();
```

Returns a reference to the one `Timeline` instance `Run()` samples every
frame — the same instance for the life of the `Application`. Use this to
read or change scale/pause from your own code; see the
[Pause & Slow Motion guide](../guides/pause-and-slow-motion.md).

## Callback types

```cpp
using UpdateCallback = std::function<void(InputManager& input, float deltaTime)>;
using RenderCallback = std::function<void(Renderer& renderer)>;
```

Plain `std::function` aliases — any callable with a matching signature
works, not just lambdas.

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
