# 2D Rasterizer — Shapes and Lines

The foundation of a software rasterizer in C++: filling rectangles and drawing
anti-aliased lines into a bitmap, with no graphics library underneath.

## What it does

Implements `GCanvas` over a raw pixel buffer, providing the primitives everything else
builds on — clearing the surface, filling axis-aligned rectangles, and drawing arbitrary
lines with anti-aliased edges.

## How it works

**Colour conversion.** Floating-point colours become premultiplied 8-bit pixels through
`color_to_pixel`, which takes an optional coverage factor. That single parameter is what
lets the same conversion path serve both solid fills and partially covered anti-aliased
edge pixels.

**Compositing.** `over` implements source-over blending, so shapes drawn with alpha combine
correctly with what's beneath them rather than replacing it.

**Line clipping before drawing.** `clip` trims a line segment to the canvas bounds *before*
rasterisation begins. Clipping first means the inner loop never tests bounds per pixel and
never touches memory outside the buffer — correctness and speed from the same decision.

**Anti-aliasing through coverage.** `plot` takes a `steep` flag alongside the coordinates
and a coverage value. Steep lines swap their x and y axes so a single loop handles both
orientations, and coverage modulates each pixel's alpha by how much of it the line actually
covers — giving smooth edges instead of stair-steps.

## Building

```bash
make
./tests
```

Requires a C++17 compiler.
