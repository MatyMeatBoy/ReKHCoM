# Controlled profiling adapter,28/10/2026

The OptiPort execution lane uses a private normal EU32/64 build from pinned
07d5e145, not the title441 fixture that disables embedded assets. Build/toolchain
receipts and input manifests remain private under the workspace's
`pruebas/optiport-full-pass-20261028/`; published2.2 packages are unchanged.

The controlled configuration is1920x1080, view4, zoom8(x4), integer scaling,
nearest filtering, windowed, vsync0, frame skip0 and one PPU worker. Graphics
mode/applied1 preserve the existing harness behavior; the four HUD/text/cards/
title flags are the only0/1 paired axis. Movies/music/scenes stay off to isolate
graphics. This is a declared workload, not a claim about personal settings.
The adapter checks the effective startup settings before interpreting samples.

Quality captures are separate from timing. Native32 uses the game readback;
freestanding64 uses the existing real host readback because its SDL shim does
not implement RenderReadPixels. Reports identify host-present counter separately
from guest/game frame and validate BMP metadata/pixel payload size. A valid BMP
does not prove matching scene/state or visual quality: image/state review still
must establish equivalence before performance comparisons. Readback/IO runs do
not supply comparable pacing results.

CPU report controls reject missing/different effective settings, missing/truncated
captures and misleading host/game counter identity. Existing finite/frame/mode/
logic-step checks remain. No renderer, game timing, resource recipe or default
of the actual application is changed by this tooling-only amendment. No new
performance or complete-quality result is claimed before the runtime cohort.
