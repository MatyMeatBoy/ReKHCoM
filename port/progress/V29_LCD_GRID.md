# Optional LCD pixel grid

`lcd_grid` is a persisted0..100 intensity, default0. Video → LCD pixel grid
is independent of GBA LCD colour correction and horizontal scanlines; existing
settings/presets retain their values. The appended PortConfig field preserves
all previous field offsets.

Two cached black/alpha textures (width×1 and1×height) form the vertical and
horizontal gutters. Coverage is the exact integral of a periodic gutter over
each physical output pixel: one eighth of a source pixel, capped at one physical
pixel. Pitch and phase follow the actual source/destination transform on each
axis, including cropped/narrow240×160 pictures, fractional fits and anisotropic
CRT geometry. At underresolved pitches the integrated mask averages mostly to
darkening; it cannot promise a crisp physical grid below the display resolution.

Geometry changes generate/upload O(width+height) pixels; one reusable temporary
buffer has4×max(width,height) bytes, cached pixels4×(width+height), each axis
bounded to32768pixels. Steady frames and intensity changes upload nothing and
use two existing SDL_RenderTexture calls. Intensity uses AlphaMod; no repeat API,
shader, bridge ABI or external bitmap is added. Off makes no texture allocations,
uploads or filter draws. No gameplay/PPU/camera/art/import data is changed.

The grid covers the useful viewport after the journal footer, before notices,
frame borders, achievements and final scene fade. All22 native renderer-owned
textures are covered by reset recovery. Allocation/setup failures retain the
picture without a stale grid; repeated failing geometry is not retried every
frame, and switching off/reset re-arms it. Previous caches are retained until
both replacements exist.

Validation:

- C32/C64:98,304 coverage cases per architecture against independent interval
  overlap; negative/fractional phases, small/integer pitches, periodic shifts,
  separate axis keys, O(w+h)uploads, zero steady/slider uploads, two draws,
  creation/update/malloc/render failures and retry ownership.
- Actual native32 config/input fixture: default0, roundtrip/clamping and
  independent title-hint/colour settings; existing input shortcuts pass.
- Native renderer-reset fixture:22texture owners, deferred detach, generation,
  target-only and thread-affinity behavior pass.
- Guest source patcher with/without DualSense passes; MMPX insertion no longer
  assumes the next enum row is LCD colour correction.
- Isolated Windows64 EU build and ABI/boot/BIOS tests pass. Four real hidden
  D3D11 captures (Auto x2 and Original fractional, off/60) exit0 with511accepted
  presents each. In Auto all changed pixels are within the useful viewport;
  border and outer pixels are identical. These loaded-host/readback runs are
  visual tests, not clean pacing measurements or physical monitor certification.
- check_sources initially reports only an existing405row stale launcher recipe
  table, unchanged by this PR. Separate PR634 synchronizes that baseline;
  full freshness gate must be repeated on the combined release source.

Private evidence and labeled before/proposal PNGs: pruebas/release-v29/issue598.
CRT physical output, other target builds and platform-specific runtime/reset
remain final-release gates. Default0 does not activate a visual proposal or
inherit Atlas approval from any existing art.
