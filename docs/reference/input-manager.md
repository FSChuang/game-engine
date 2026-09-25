# InputManager

<div class="ge-meta" markdown>
<span class="ge-meta__item"><span class="ge-meta__label">Header</span><span class="ge-meta__value"><code>Engine/Input/InputManager.h</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Namespace</span><span class="ge-meta__value"><code>Engine</code></span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Category</span><span class="ge-meta__value">SDL</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Thread Safety</span><span class="ge-meta__value">Single-threaded (main thread)</span></span>
<span class="ge-meta__item"><span class="ge-meta__label">Blocking</span><span class="ge-meta__value">Never</span></span>
</div>

For the mental model, see [Input State](../concepts/input-state.md). This
page is deliberately just the API facts.

## Synopsis

```cpp
namespace Engine
{
	class InputManager
	{
	public:
		void Update();
		bool IsKeyPressed(SDL_Scancode key) const;
		bool IsKeyJustPressed(SDL_Scancode key) const;
	};
}
```

No constructor is declared — `InputManager` uses its implicit default
constructor, which only zero-initializes its internal snapshot array.

## SDL_Scancode

Every method takes an `SDL_Scancode` — SDL's *physical key position* type
(e.g. `SDL_SCANCODE_W` is always the key in the "W" position on a QWERTY
layout, regardless of the user's actual keyboard layout), not a
layout-dependent keycode or a text character.

## InputManager::Update

```cpp
void Update();
```

Copies SDL's current keyboard state into the previous-state snapshot that
`IsKeyJustPressed` compares against. Must be called exactly once per frame.
`Application` calls it **last** in the frame — see
[Input State](../concepts/input-state.md) for why that ordering matters.

## InputManager::IsKeyPressed

```cpp
bool IsKeyPressed(SDL_Scancode key) const;
```

True while `key` is currently held down. Queries SDL's **live** keyboard
state on every call (`SDL_GetKeyboardState`) — never a cached value.

## InputManager::IsKeyJustPressed

```cpp
bool IsKeyJustPressed(SDL_Scancode key) const;
```

True only on the frame `key` transitions from not-pressed to pressed:
`IsKeyPressed(key) && !` (key was down in the last `Update()` snapshot).
False while the key continues to be held, and false again once it's
released.

## Per-frame semantics

"Current" (`IsKeyPressed`) is always live. "Previous" is whatever `Update()`
last recorded. See [Input State](../concepts/input-state.md) for a worked
frame-by-frame table of held vs. just-pressed.

## Example

```cpp
[](Engine::InputManager& input, float deltaTime)
{
    if (input.IsKeyPressed(SDL_SCANCODE_RIGHT))
    {
        player.Move({ speed * deltaTime, 0.0f });
    }

    if (input.IsKeyJustPressed(SDL_SCANCODE_SPACE))
    {
        // Trigger a jump exactly once per press, not once per frame held.
    }
}
```

## See Also

- [System: Input](../systems/input.md)
- [Concept: Input State](../concepts/input-state.md)
- [Concept: Application Loop](../concepts/application-loop.md#input-update-ordering)
