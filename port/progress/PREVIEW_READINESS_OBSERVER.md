# Gallery preview readiness and optional observation

Product source verified: `1af8a385f54e0a74966bee83c300d617e2ddc7a0`. This is the scoped `preview-readiness-observer` child of the still-open `optiport-v22-async` area.

A preview CPU buffer did not prove a usable replacement texture. The native draw previously ignored upload/draw rejection and consumed the texture serial even after failed upload. The bridge could acknowledge a CPU copy while its void host update failed, leaving selection A drawn over selection B. Production-function RED tests reproduce both cases.

Keep the selected CPU still beneath its optional full-resolution overlay. Check upload/draw acceptance and retry rejected uploads. For bridge guests, recreate only when CPU content changes, so the initial host upload must succeed; stable frames reuse their texture. A fallback may be less sharp than the overlay. It preserves the same selected image, frame, layout and fade; no art/media/default changes are included.

## Verified scope

- Production C contracts cover ten lifecycle scenarios, including the74/75ms debounce boundary, missing/corrupt media, stale generations, pending closure and deliberate final cleanup. Two deliberately broken lifecycle variants are rejected.
- Native draw, guest upload/HostTex and host-slot functions cover first creation/upload rejection, A-to-B stale upload, retry and stable-frame reuse. Underlay and disabled-observer negative controls are rejected.
- Existing closed EU32/64 producer objects were read only. Only three private translation units per architecture were recompiled; source/recipe/output hashes are retained. The five normal producer output hashes remain unchanged.
- Final functional evidence contains19 completed executions:14 authoritative cases and five superseded observer attempts, all preserved. Actual local D3D11 output matches baseline07d5 pixel for pixel in100 sampled frames, within each architecture, including menu entry/closing fades and letterboxing.
- Explicit host upload rejection on the first legal preview leaves the selected CPU still visible. Switching EN to JP through the real options Change path changes serial/path generation. Rejecting the second host upload displays the new purple JP fixture, with no old blue preview pixels, then retries successfully.
- Native32 cold playback keeps GBA visible, starts the real Media Foundation one-frame decoder and accepts HD before closing. Closing with an explicitly delayed worker pending rejects late acceptance; process shutdown deliberately reaps it. Hybrid selection is exercised in32/64.

Private evidence: `pruebas/optiport-v22-async/FINAL-VALIDATION.json`, `FUNCTIONAL-CANDIDATE-BUILD.json`, `POSITIVE-GPU-PIXELS-1af.json`, `FIRST-REJECTED-STILL-64.json` and the retained raw run directories. The final process inventory is quiescent.

## Observer and limits

`KHCOM_PREVIEW_TRACE` enables at most256 in-memory events. Shutdown writes `preview_trace.csv`; overflow is counted. Disabled observation performs no clocks or file I/O. Records distinguish request, CPU fallback/cache readiness, worker start, polled HD readiness, upload/draw submission, underlay presentation return and overlay presentation return. Only a path-generation hash is recorded, not the player's absolute media path.

CPU-ready is observed when polled, not at the decoder's completion instant. Bridge returns describe submissions, not GPU completion or physical scanout. Guest logic frames stand still under menus; screenshots use UI ticks or actual host presentation counts. Functional tests use isolated legal fixtures, hidden owned windows and explicit injected failures. They are not performance comparisons, overhead measurements, a soak, or physical Steam Deck confirmation.

Actual window sizes differ between the existing32/64 paths despite the same requested INI. No cross-architecture image or latency equivalence is claimed. Cold64 has no decoder worker when its sidecar is absent and retains GBA; parity needs a separately negotiated host-preview contract. Area9 remains open. Comparable cold/warm latency and observer overhead are also pending, after effective configuration parity and a timing lease.

The superseded fixtures exposed observer errors: `KHCOM_OPTIONS_SHOT` opened an unintended first menu; the patched64 enum adds an MMPX row (JP125 in native32,126 in the guest); and an early cold run closed before acceptance. Corrected runs preserve those originals rather than rewriting their results.
