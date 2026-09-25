# Renderer

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Renderer/Renderer.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">SDL</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">Single-threaded</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Ownership</span><span class="ge-meta__value">Owns SDL_Window / SDL_Renderer</span></span>
</div>

For the mental model, see [Rendering & Scaling](../concepts/rendering-and-scaling.md).
This page is deliberately just the API facts.

## Synopsis

```cpp
namespace Engine
{
	struct WindowConfig
	{
		std::string Title;
		int Width;
		int Height;
	};

	enum class ScalingMode
	{
		Constant,
		Proportional
	};

	Vector2 ApplyScalingMode(Vector2 value, ScalingMode mode,
	                          Vector2 referenceResolution, Vector2 currentResolution);
	ScalingMode NextScalingMode(ScalingMode mode);

	class Renderer
	{
	public:
		explicit Renderer(const WindowConfig& config);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		void BeginFrame();
		void EndFrame();
		void DrawEntity(const Entity& entity);

		void SetScalingMode(ScalingMode mode);
		ScalingMode GetScalingMode() const;
		void ToggleScalingMode();
	};
}
```

## WindowConfig

```cpp
struct WindowConfig
{
	std::string Title;
	int Width;
	int Height;
};
```

Passed to the `Renderer` constructor. `Width`/`Height` set the initial SDL
window size **and** become the fixed *reference resolution* that
`Proportional` scaling measures against for the renderer's entire lifetime —
there is no way to change the reference resolution after construction.

## ScalingMode

```cpp
enum class ScalingMode
{
	Constant,      // Rendered directly in pixel units; unaffected by window resizing.
	Proportional   // Scaled by currentWindowSize / referenceResolution.
};
```

How an entity's position/size are mapped onto the current window size. See
[Rendering & Scaling](../concepts/rendering-and-scaling.md) for the exact
math and worked examples.

## ApplyScalingMode

```cpp
Vector2 ApplyScalingMode(Vector2 value, ScalingMode mode,
                          Vector2 referenceResolution, Vector2 currentResolution);
```

Pure function, independent of SDL and unit-testable without a window.
`Constant` returns `value` unchanged. `Proportional` scales each axis
independently: `value.X * (currentResolution.X / referenceResolution.X)`,
and the same for `Y`. Does not read or modify any `Entity` or `Renderer`
state itself — `Renderer::DrawEntity` is what calls it, once for position and
once for size.

## NextScalingMode

```cpp
ScalingMode NextScalingMode(ScalingMode mode);
```

Returns the other mode (`Constant` ↔ `Proportional`). Pure logic backing
`Renderer::ToggleScalingMode`, kept separate so it's testable without a
window.

## Renderer

```cpp
explicit Renderer(const WindowConfig& config);
~Renderer();
```

Creates the SDL window and renderer together via
`SDL_CreateWindowAndRenderer`. If creation fails, whichever handle *did*
succeed is destroyed before `std::runtime_error(SDL_GetError())` is thrown —
construction never leaves a partial resource behind. The destructor
destroys the renderer, then the window, unconditionally.

Neither copyable nor movable.

## Renderer::BeginFrame / EndFrame

```cpp
void BeginFrame();
void EndFrame();
```

`BeginFrame` clears the frame to the engine's fixed background color.
`EndFrame` presents it (`SDL_RenderPresent`). Call `BeginFrame`, then any
number of `DrawEntity` calls, then `EndFrame` — exactly the order
`Application::Run` already uses around your render callback.

## Renderer::DrawEntity

```cpp
void DrawEntity(const Entity& entity);
```

Draws `entity` as a filled rectangle using its position, size, and color.
Reads the **current** window size fresh on every call (`SDL_GetWindowSize`),
applies `ApplyScalingMode` to both position and size using whatever
`ScalingMode` is currently set, and calls `SDL_RenderFillRect`. Takes no
ownership of `entity` — nothing about it is stored.

## Renderer::SetScalingMode / GetScalingMode / ToggleScalingMode

```cpp
void SetScalingMode(ScalingMode mode);
ScalingMode GetScalingMode() const;
void ToggleScalingMode();
```

Plain state mutators/accessors — no side effects beyond storing the mode.
`ToggleScalingMode()` is equivalent to
`SetScalingMode(NextScalingMode(GetScalingMode()))`. The new mode takes
effect on the very next `DrawEntity` call; nothing already drawn is
retroactively affected.

## Ownership and resource notes

`Renderer` owns its `SDL_Window*`/`SDL_Renderer*` for its entire lifetime and
exposes neither to callers. It holds no reference to any `Entity` — every
`DrawEntity` call reads its argument once and stores nothing.

## Minimal example

```cpp
application.Run(
    [](Engine::InputManager& input, float deltaTime)
    {
        // Update game state here.
    },
    [](Engine::Renderer& renderer)
    {
        renderer.DrawEntity(player);
        renderer.DrawEntity(wall);
    });
```

## See Also

- [System: Renderer](../systems/renderer.md)
- [Concept: Rendering & Scaling](../concepts/rendering-and-scaling.md)
- [Guide: Toggle Scaling Mode](../guides/toggle-scaling-mode.md)
- [Reference: Entity](entity.md)
