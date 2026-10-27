# Saved-game first door: portable BIOS read

The Windows x86/x64 preview at source `cee5b17516ebbbb34ac76715929d856e5f854c47`
crashed entering the first room after loading the user's SRAM. Forcing x86
worked. Two Windows Application Error events reported `0xC0000005` at
`0x100f7bd6`, inside the exact EU guest's `MapGmkFindLeftWallFace3x3`.
Disassembly identifies `testb $1, 1(%eax)` following `MapCellAt`: the inlined
`MapWallFaceIsUnreserved` reads the cell's reserved flag.

The scan queries `y - i` and can reach negative rows. `MapGetCell` correctly
returns NULL there. The cartridge reads BIOS; the native x86 exception handler
models that read, but the portable guest has no such recovery. This is not
evidence of insufficient RAM, an ABI calling-convention fault or HD exhaustion.

The shared source now uses `BIOS_PTR` at this specific read. The BIOS value
`0xE3A02004` has no reserved bit and its cell type ends the column scan. Returning
false instead would change placement; changing `MapGetCell` globally would break
consumers that rely on NULL. Both left/right wall placement paths call this
helper. Other unrelated reads require their own contract and evidence.

## Evidence

- `port/tools/test_map_wall_bios.py` compiles the actual helper without exception
  recovery. Before: `0xC0000005`. After: exit 0. Covers three columns scanning
  beyond row zero, reserved cells and valid termination; BIOS data stays intact.
- Isolated copied user SRAM, EU, automatic playthrough bot, HD settings retained
  apart from window/vsync/test isolation: before fix exit `0xC0000005` after
  loading slot 0; after fix reaches `Mode_MapFld` at frame 520, floor 3/world 5/
  room 0 and exits 0 at frame 2800. The bot subsequently stalls navigating;
  this proves the reported transition, not a complete playthrough.
- Private backup hashes, original logs and red/green outputs live outside the
  protected Quiver installation in ignored `build/user64-door-20261027`.
  Do not publish the user's save or authentication/configuration files.

## Release coverage

Rebuild every target from the shared fix. Compare x86 and x64 with the same
save and transition, then run quick room/seed, door, battle, media and resource
failure scenarios. The user's requested gate is broad pattern-based regression,
not playing the entire story. Record platform and renderer limits honestly;
passing this scenario alone is insufficient evidence for a stable 2.1.
