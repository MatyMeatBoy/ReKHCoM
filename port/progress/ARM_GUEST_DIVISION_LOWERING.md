# Shared ARM guest arithmetic producer

The shared `build_switch.compile_all` now lowers emitted scalar i32 game
DIV/REM before LLVM optimization, using the contract verified in
[the cross-target audit](ARM_WASM_DIVISION_PARITY.md). This producer is reused
by Switch, Android64, LinuxARM64 and Vita. Only C sources under the game's
`src/` tree take the two-stage path; port/host/assembly units remain unchanged.
Web, Android32 CMake, Anbernic and x86 producers are outside this change.

The frontend keeps the existing target/CPU flags and optimized-mode attributes,
disables LLVM passes long enough to establish the arithmetic contract, and
retains its original header dependency file. O2 code generation then consumes
the guarded IR without overwriting those dependencies. The existing 32-bit
pointer and game-section conventions remain in effect.

Game cache markers include source identity, source bytes, compiler identity,
flags and producer/helper hashes. An old object without a matching marker
must rebuild. A failed frontend or backend leaves no valid marker. Header
dependency checks retain escaped-space support and reject missing inputs.

Six regressions use real Clang: game-only scope, same-input cache reuse,
pre-lowering object upgrade, header dependencies and changed flags/helper,
changed source with preserved timestamps, and failed frontend/backend cache
invalidation. The preserved-timestamp case failed before the source hash was
included and passed afterward.

The actual producer compiled acgtrans and bg_anim with each of the four
configurations: eight game objects. Linked fixtures from the same producer
also ran in diskless CPU emulation for each configuration, checking ILP32,
a fixed global alias, an indirect integer callback and five ordinary/zero/
signed-overflow arithmetic cases. AArch64 used actual linked arm64_32 Mach-O
segment bytes; Vita used its compiled Thumb ELF with compatible integer
compiler-rt routines. The bare-metal output driver is separate from the
unchanged target flags of the fixture.

These are producer/ABI CPU controls, not console/device certification, a full
guest-platform build, or a player-crash reproduction. Floating callbacks and
application lifecycle are not covered. Android32/Anbernic integration and the
parent parity audit remain open. Published v2.2 packages and live Web delivery
are unchanged by this source PR.
