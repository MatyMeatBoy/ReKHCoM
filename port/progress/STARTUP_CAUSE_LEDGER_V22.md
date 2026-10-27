# Startup cause ledger: Deck #330 and Windows #360

Diagnostic plan, 2026-10-27. Neither reporter's cause is established. This is a
source review and a CPU-only fallback predicate check, not device confirmation.
Owner: `codex-startup-ledger`; queue task `v22-startup-cause-ledger`.
The separate `codex-deck-entry-diagnostic` claim remains untouched.

## Evidence and version boundary

[#330](https://github.com/MatyMeatBoy/ReKHCoM/issues/330) and its
[2026-10-27 follow-up](https://github.com/MatyMeatBoy/ReKHCoM/issues/330#issuecomment-6446302893)
say native 2.1 works from Konsole but fails in Gaming Mode after setup;
Documents differs from the tutorial example. No new error log or shortcut
fields establish the failing phase. Starting directly in the game is expected
for this native shortcut; the Desktop importer is a separate entry point.
[#360](https://github.com/MatyMeatBoy/ReKHCoM/issues/360)'s original body says
Automatic and Force 64-bit fail while Force 32-bit works on Windows 11 x64.
It supplies no exception, GPU, process-lifetime trace or exact installed hashes.
The unrelated Android comment cannot identify Pedro's Windows failure.

Recovered 42 primary source files from the timed-out investigation. Independently
compared every cached file against `git show ref:path`: all match after CRLF/LF
normalization. The cache is evidence of source content, not a completed diagnosis.
The following ledger and falsification plan are this reviewer's analysis.

Published tag: `8c613ae5d`; supplied motor provenance: `cd144335e`; supplied
launcher/Linux packaging provenance: `de434db7c`. Release host, common main,
wrapper and runtime-predicate blobs were checked against those source refs.
Comparison target is pinned `61aee4c40`, not an ever-moving main. Documentation
branch starts at `8953b0d35`; no runtime edits are proposed here.

| File | Release blob | Comparison at 61aee4c40 |
| --- | --- | --- |
| `port/linux/ReKHCoM.sh` | `43c2aeee643dab5a20f8b2b7fdb75b095e6dd7e4` | Forwarding game arguments added by #376 |
| `port/linux64/linux64_host.c` | `c21699689e4a39562937ceecf2c90d472e24fa21` | Argument adapter added by #376 |
| `port/windows64/windows64_host.c` | `857cb0463389ce96dd3cbd7aca6b6644c667ce68` | Crash-reporting #368 and CPU preflight #377 added |
| `port/decompui/runtime_selection.h` | `f7b1c748771d9036b2b4212fc0a5c68843b1525c` | Exception/recovery and CPU policy changed |
| `port/src/port_main.c` | `f68cd2aeeb39850e4b6698861d52cf04f57aae03` | Identical |
| `port/tools/build_linux.py` | `09c4e3f230ae01acc77aad39b62f870bfd0dd580` | Identical |

The release Setup script is `b00a96e0d0bff9bc0241cee691111ca6c6469d67`,
also unchanged at the comparison target. It differs from the earlier motor ref
only in instructions about the Desktop importer. #376's language contract and
#377's CPU guard are existing work, not newly established causes of these reports.
#368 adds recovery/reporting; recovering in 32 bits does not explain the crash.

## Source anchors and real phase boundaries

- [Release wrapper lines 7-48, 88-106, 137-150](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/linux/ReKHCoM.sh#L88-L150): own-directory resolution, desktop registration, log redirection, inherited library environment and fallback.
- [Release Windows launch lines 3417-3474](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/decompui/decompui.cpp#L3417-L3474): separate child process, hidden launcher, wait, marker and retry.
- [Release retry predicate](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/decompui/runtime_selection.h#L10-L14): pre-entry codes 2/3/4 and three Windows loader statuses qualify; arbitrary exceptions do not.
- [Windows host lines 498-570](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/windows64/windows64_host.c#L498-L570) and [Linux host lines 348-438](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/linux64/linux64_host.c#L348-L438): map/init/window/renderer precede marker; marker precedes guest call.
- [Guest entry](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/ps5/guest/src/platform_ps5.c#L43-L56) calls common main. [Guest renderer shim](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/ps5/guest/src/sdl_shim.c#L206-L251) probes the host through `BRIDGE_GPU_PROBE`.
- [HostGpuAvailable](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/ps5/shared/host_gpu_targets.h#L18-L33) performs real target-texture creation and premultiplied blend setup. This occurs **after** the entry marker, before successful first presentation is established. A normal unsupported capability returns false; that alone is not a crash.
- [Common initialization](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/src/port_main.c#L3360-L3471) includes config, shim renderer, ROM and optional HD warmup. [Host present](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/windows64/windows64_host.c#L231-L243) is a later boundary.
- [Linux SDL build recipe](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/tools/linux_prepare.py#L130-L141) enables X11/KMSDRM, disables Wayland/Vulkan. The recipe constrains the first backend differential; it does not replace inspection of the installed SDL artifact or prove which backend ran.

Do not rename `guest-entered` to GPU-ready/first-frame. A marker proves the host
reached its pre-call write, not even that the first guest instruction completed.
A successful host renderer creation does not prove later texture/driver operations
or presentation. No log can mean redirect/open failure or execution never reached
the logger; read-only installation is one discriminator, not a conclusion.

## Cause ledger and minimum falsification

Priority here is information gain, not estimated probability or a promised fix.
All rows remain hypotheses unless their explicit control is observed.

| Candidate / report | Evidence for | Evidence against or missing | Smallest discriminating check |
| --- | --- | --- | --- |
| Steam never executes the intended wrapper (#330) | Terminal versus Gaming Mode differs; Steam shortcut fields have not been captured. | Own-directory and quoted desktop registration exist. Existing fake-executable shell fixtures pass paths with spaces/metacharacters, but do not exercise Steam/GIO parsing. Documents alone is not a defect. | Record actual Target/Start In/options and forced-compatibility state; launch the SAME registered shortcut from Steam Desktop then Gaming Mode. Put an opt-in pre-redirection sentinel in a writable temporary diagnostics location; record wrapper identity and invocation, without environment secrets. If sentinel appears, reject the no-wrapper hypothesis for that run. |
| Native runtime / inherited loader environment differs (#330) | Wrapper prepends bundled lib64 while retaining LD_LIBRARY_PATH and LD_PRELOAD; terminal/Steam environments can differ. | No loader error or conflicting library identified; shell success is not a real graphics test. Native Linux must be distinguished from a Windows binary under Proton. | Capture resolved executable, native ELF architecture, loader error, backend/driver and a narrow allowlist of launch variables in each of the three contexts. Change ONE observed variable only in an isolated test; restoration must restore the failure. Never prescribe LD_PRELOAD= or Proton changes as a universal fix. |
| Host or post-entry GPU initialization/presentation fails (#330, #360) | Host renderer creation and later probe texture/blend are distinct driver boundaries; entry marker does not exclude driver failure. | No renderer name, exception address or last completed GPU operation from either device. Unsupported target capability normally falls back. Terminal success weakens a machine-wide unsupported-GPU theory for #330. | Record begin/end/result/SDL error around host renderer, GPU probe, and first present. Same binary/config/assets, isolated saves; compare contexts or32/64. A completed first present rejects startup-before-present for that run, but not hidden/wrong-window display. |
| Child remains alive while launcher is hidden (#360) | Published launcher hides its window and waits without a startup deadline; a stalled child can look like launcher closure. | Report calls it a crash but gives no process state. Genuine child exit should reshow launcher except explicit quit-all. | Observe launcher PID and child PID during failure. Live child + live hidden launcher supports stall; an exit code falsifies a continuing wait. Preserve phase and elapsed time; do not force-kill original sessions or automatically retry a still-running child. |
| Exception or non-retry exit in64 (#360) | Auto and Force64 both failing while Force32 works fits an exception excluded from release retry, or an error after marker. | No exception code/address; not proof of ABI, heap, OOM or driver cause. Windows11 x64 alone does not establish capability of every path. | Use existing #368 report on a clearly identified diagnostic build; retain raw code, phase, module/guest offset and exact host/guest/map hashes. Reproduce same ROM/config/assets; map address to matching build before selecting driver versus guest/bridge investigation. |
| Normal handled pre-entry file/map/renderer error (#360) | Those paths return2/3/4 in release; plausible for incomplete package or map failure. | Auto should retry32, and reporter says forced32 works. A plain recognized return alone explains Force64 better than both modes. Stale/wrong install or actual fallback32 failure still possible. | Record selected runtime, every child start/exit, marker and retry. One observed2/3/4 before marker followed by working32 falsifies this as explanation of Auto failing. Do not apply newer main policy retroactively. |
| Guest-specific asset/font/heap or bridge path (#360; possible #330) |64 guest uses shim, its own memory and HD text implementation; failure may occur after entry but before frame. | No failing input, allocation, address or boundary identified. Existing mixed-ABI probes are counterevidence to a generic ABI claim, not exhaustive coverage. | After locating guest phase, compare isolated cold sessions with identical inputs and optional HD disabled, recording allocation/asset failure and address. Success only with HD disabled narrows a path, not the precise defect. Only then create a focused CPU fixture or decoder/ABI regression. |

A visible shortcut that does not enter, an absent shortcut, a process with a hidden
window, and a process exited before logging require different branches of this
plan. A first-present success with no visible content calls for window identity,
visibility/focus/output capture, not a repeat of loader diagnosis. Similar issues
in other recompilations provide possible discriminators, not transferable fixes.

## CPU-only check and next bounded observation

Executed a tiny C++ fixture including the **unchanged release predicate header**,
compiled with clang++ (llvm-mingw). It asserted2/3/4 plus the three documented
loader codes retry before entry, none retry after entry, and access violation
0xc0000005 and success0 do not retry: **14 assertions passed**. This falsifies
the claim that published Auto already retries every early exception. It does not
exercise CreateProcess, marker IO, actual launcher control flow, SDL or a reporter
machine. The fixture uses only the production inline function:

```cpp
unsigned codes[] = {2,3,4,0xc0000135u,0xc000007bu,0xc0000142u};
for (unsigned c : codes) {
    assert(ReKHCoMRetry32(c, false));
    assert(!ReKHCoMRetry32(c, true));
}
assert(!ReKHCoMRetry32(0xc0000005u, false));
assert(!ReKHCoMRetry32(0, false));
```

The first useful device observation is a single isolated attempt per context,
with process lifetime and ordered phases. Preserve original logs and bounded
history, artifact hashes and configuration fingerprint; keep private absolute
paths/environment values out of public evidence. If instrumentation is needed,
add opt-in bounded begin/end records for wrapper redirect, child start/exit,
host renderer, pre-guest call, GPU probe and first present, with monotonic elapsed
time and success/error. Flush each startup boundary; log failure must not crash
the game. Reuse existing #368 crash reporting rather than create another crash
handler. No per-frame spam, forced backend, automatic post-entry recovery or
hardware workaround is justified by this ledger alone.

No new product bug or causal fix is claimed. No GPU, VM, full build, actual Steam,
GIO registration or reporter installation was exercised in this pass. Existing
shell fixtures and ABI probes remain separate evidence supplied by the coordinator;
they were not rerun here. No reporter comments or releases were published.
