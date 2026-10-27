# Optional bridge GPU targets (v2.1)

The existing `HF_thread` slot transports an optional protocol; `HF_COUNT`, the
bridge header and opcodes 0..4 are unchanged. Windows64/Linux64/Android64 opt in.
Other hosts answer zero to unknown opcodes and retain direct rendering.

| Opcode | Meaning |
| --- | --- |
| 0x100 | target capability probe; exact `BRIDGE_GPU_MAGIC` required |
| 0x101 | create target, width/height in a/b; opaque generation handle or -1 |
| 0x102 | bind target handle a, or -1 for window; success 1, failure 0 |
| 0x103 | clear active output to ARGB a, including alpha; real SDL clear |
| 0x104 | draw blend enum a (NONE/BLEND/ADD/PREMULTIPLIED) |
| 0x105 | real renderer maximum texture size, or zero |
| 0x106 | scratch target budget in bytes |
| 0x107 | texture handle a uses linear (b=1) or nearest (b=0) sampling |
| 0x108 | protocol identity, even when targets are unsupported |

Reserved getenv metadata queries report `KHCOM_GPU_RENDERER` (actual SDL name)
and `KHCOM_GPU_TARGETS` (version/max/budget). The shim requires the protocol
identity before trusting renderer metadata, so an old host's ordinary environment
cannot manufacture capability. Old hosts report `bridge-unknown`; software hosts
report their actual name and direct rendering. No fallback claims AA parity.

Targets use the existing 12-bit slot/19-bit allocation serial and renderer epoch.
Each has one GPU ID for both filtering modes, no CPU pixel buffer, and cannot be
updated through SDL_UpdateTexture. Free/reset first unbinds an active target;
bytes are released exactly once. The host enforces cumulative scratch budgets:
32 MiB desktop, 12 MiB Android or HOST_GPU_CONSTRAINED. This is not a total GPU
memory limit. Creation failure is eager, allowing the complete gauge chain to
roll back before drawing.

The shim tracks viewport, clip and scale per destination. Its coordinates are
already transformed in Corner, so the host must not apply a second viewport
transform. Opening a frame reapplies the shim clip after gfx_begin; binding
restores destination state and presenting restores the window. Device resets
invalidate GPU contents and handles; the gauge redraws the full chain each frame.

Validation is deliberately split. `test_bridge_gpu_targets.py` links the complete
production shim, shared host target/slot code and actual Windows H_gfx_draw with
mock SDL services; it tests legacy/new protocol traffic, unsupported software,
12/32 MiB budgets, allocation failures, stale epochs, rollback, transparent clear,
premultiplied draw state and viewport/clip/scale. Existing texture generation and
gauge fault-injection tests remain applicable. This does not validate physical
GPU filtering or pixels: root must compare the gauge on Windows D3D, Linux GPU
and Android emulator/device before claiming visual parity.

## Failed unbind and restoration ownership

Destroying an active target is deferred if restoring the window fails. The guest
keeps its Tex object and renderer alias alive; the host retains the texture slot
and charged bytes. A later successful bind drains deferred destruction. Failed
binds block clear/draw/present until an explicit bind recovers the output. Reset
also retains a still-bound target when unbinding fails, rather than invalidating
a live renderer pointer or silently releasing its budget.

The gauge does not composite after a failed destination restore. It retains a
reference to its original destination, retries restoration before more gauge
work, and preserves the scratch cache if shutdown cannot yet restore safely.
The failure paths are covered by active-destroy, host-free, reset, persistent
shutdown, every bind ordinal and recovery tests. Physical GPU QA is still pending.
