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

## API Reference

<!-- Generated from Engine/src/Engine/Input/InputManager.h by
     scripts/generate_api_docs.py — do not hand-edit the section below; edit
     the header's /// comments instead and regenerate. -->

--8<-- "input-manager-api.md"

## Behavior notes

No constructor is declared — `InputManager` uses its implicit default
constructor, which only zero-initializes its internal snapshot array.

Every method takes an `SDL_Scancode` — SDL's *physical key position* type
(e.g. `SDL_SCANCODE_W` is always the key in the "W" position on a QWERTY
layout, regardless of the user's actual keyboard layout), not a
layout-dependent keycode or a text character.

"Current" (`IsKeyPressed`) is always live — it queries SDL directly
(`SDL_GetKeyboardState`) on every call, never a cached value. "Previous" is
whatever `Update()` last recorded, and `Application` calls `Update()`
**last** in the frame — see
[Input State](../concepts/input-state.md) for a worked frame-by-frame table
and why that ordering matters.

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
