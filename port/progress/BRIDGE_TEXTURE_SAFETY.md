# Shared guest texture failure contracts

The shared SDL shim returns NULL when a CPU/GPU texture allocation fails,
but its update/modulation/filter functions and rotated draw dereference that
pointer before validating it. Optional overlay callers invoke these functions
after creation without a guard; real SDL rejects the operation safely.

Reproduce using the complete production shim and synthetic allocation faults,
without a ROM/device. Cover both allocation stages, all related texture
operations, surface conversion, rectangle clipping and pitch/size overflow.
Retain valid pixel conversion, modulation, filtering, upload and draw results.
Compare update clipping with upstream SDL3.4.18 SDL_render.c.

Return an error for rejected operations; never fabricate a usable texture or
read/write beyond its pixel allocation. Preserve existing native fallback
selection. This is a proven potential allocation-failure crash pattern, not
the established cause of any specific external report.
