# Runtime architecture

Windows keeps ReKHCoM.exe and its SDL3.dll 32-bit. bin/ keeps 32-bit games;
bin64/ contains rekhcom64.exe, 64-bit SDL3.dll and guest-{eu,us,jp}.bin.
Never put a 64-bit DLL beside the 32-bit launcher. ROMs, settings and manual
saves use the same data root.

Settings > Graphics > Advanced > 32/64-bit mode persists runtime_mode in
decompui.ini: 0 Automatic (default, prefer x64), 1 Force 32-bit, 2 Force 64-bit.
Forced 64-bit reports an incompatible/missing runtime or initialization failure
without retrying 32-bit. Automatic retries only process creation/loader or
explicit host initialization errors before the --boot-status marker exists.
Hosts create this marker immediately before entering guest boot. Elapsed time
is never used. Guest crashes and quits (including exit 81) never start a retry.

Linux ReKHCoM.sh reads the same preference, with --auto, --force32 or --force64
before --region for a one-run override. REKHCOM_64=0 still forces 32-bit.
Android installation selects its native ABI; it has no architecture toggle.
Quick states are architecture-specific and incompatible signatures must fail
safely. Manual cartridge saves share the data directory.

The 64-bit hosts remain alpha. HD/video parity requires platform validation;
selecting 64-bit does not certify feature parity.
