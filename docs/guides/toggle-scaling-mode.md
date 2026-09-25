# Toggle Scaling Mode

## Goal

Switch how your entities are scaled to the window between **Constant** and
**Proportional**, at runtime.

## Prerequisites

- An `Application` already constructed and running, drawing entities via
  `Renderer::DrawEntity` from your render callback (see the
  [Quick Start](../index.md#quick-start)).

## Try it immediately — no code required

`Application` already wires up a debug key for this: press **Tab**. If you
have entities being drawn, you'll see them jump between their fixed pixel
position/size and their window-relative scaled position/size.

The rest of this guide is for triggering the same switch from your *own*
code — a settings menu, a resolution-change handler, and so on.

## Minimal example

```cpp
[](Engine::Renderer& renderer)
{
    renderer.SetScalingMode(Engine::ScalingMode::Proportional);
    // ...or, to flip whatever the current mode is:
    renderer.ToggleScalingMode();
}
```

## Step-by-step

1. **Get the `Renderer&`.** Your render callback (the second argument to
   `Application::Run`) already receives it — there's no separate accessor to
   look it up elsewhere.
2. **Call `SetScalingMode` for an exact mode, or `ToggleScalingMode` to
   flip.** Both take effect immediately for the next draw.
3. **Nothing else to do.** Every subsequent `DrawEntity` call reads the
   current mode itself — there's no separate "apply" step and nothing to
   re-render manually.

## Why it works

`DrawEntity` reads `GetScalingMode()` and the window's *current* size fresh
on every call — it never caches either. Changing the mode is just storing a
new enum value; the very next `DrawEntity` call picks it up automatically.
See [Rendering & Scaling](../concepts/rendering-and-scaling.md) for the exact
math each mode applies.

!!! warning "Proportional mode does not preserve aspect ratio"
    `Proportional` scales the X and Y axes **independently** by
    `currentWindowSize / referenceResolution`. If the window is resized to a
    different aspect ratio than the reference resolution it was constructed
    with, entities visibly stretch or squash — there is no letterboxing or
    aspect-ratio correction anywhere in `Renderer`.

## Common mistakes

- **Expecting `Constant` mode to respond to window resizing.** It doesn't,
  by design — it returns every value unchanged regardless of the current
  window size.
- **Assuming a mode change affects entities already drawn this frame.**
  Each `DrawEntity` call reads the scaling mode for itself; there's no
  retroactive re-render of earlier draws in the same frame.

## Related APIs

- [`Renderer::SetScalingMode` / `GetScalingMode` / `ToggleScalingMode`](../reference/renderer.md#renderersetscalingmode-getscalingmode-togglescalingmode)
- [`ApplyScalingMode`](../reference/renderer.md#applyscalingmode)

## Next steps

- Read [Rendering & Scaling](../concepts/rendering-and-scaling.md) for the
  exact math and worked examples.
- Read [Reference: Renderer](../reference/renderer.md) for the full API.
