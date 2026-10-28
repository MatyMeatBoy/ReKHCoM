# Launcher font atlas startup fallback

Five ARM64 tombstones from issue374 contain SIGSEGV at relativePC0x2091f0 in
ImGui::NewFrame, accessing address0x48. Launcher BuildID
e540c88b6fc13955c5454595cee941acf7618e74 matches the published stable/options
library. Instruction/layout and memory-dump checks identify a live ImFont with
ContainerAtlasNULL, FontSize0 and SourcesCount2. This is after SDL started.
The particular font file that failed on the Moto is not established.

The actual ImGui1.91.9b consumer reproduces this state when a font merge makes
Build return false. AddFont returning a pointer is not evidence of a built font.
With assertions disabled, the unchecked NewFrame performs an invalid access.

LoadFonts now resets aliases before clearing an older atlas. FinalizeFonts
builds the configured atlas before either frame consumer or backend startup,
checks every font/role/default by membership before dereferencing it, and keeps
valid configured fonts. A failed build or incomplete role set clears all aliases
and retries once with ImGui's incorporated font. A failed fallback returns via
the existing startup-error/cleanup path before NewFrame. No signal handler
attempts to continue after memory corruption.

This shared launcher code covers Windows, Linux and Android; the scene-shot
consumer and normal frame loop use the same validated atlas. The incorporated
fallback can lose custom lettering and Japanese glyph coverage. A missing font
is not a reason to claim equivalent presentation quality.

## Focused verification

- Actual ImGui release-mode NewFrame: valid font control, invalid merged sfnt,
  absent fonts, partial roles, stale role pointer, two refused builds and stale
  FontDefault. The bad merge on the old path exits0xc0000005; the valid path
  succeeds. A known valid core KHMenu.otf control keeps its configured font.
- Actual startup sections: ten failure boundaries plus success, with/without
  an error popup, on desktop GL and Android GLES fixtures. Total font failure
  cleans up once without initializing downstream backends or entering frames.
- `test_launcher_font_atlas.py` uses source-fingerprinted ImGui object caches
  only; caller/helper is recompiled for each run. CPU probes are distinct from
  GPU, Android installation and physical Moto/S22 validation.

Private evidence and exact source/build receipts remain under
`pruebas/moto374-safe-startup-current/` and the task-specific D: QA directory.
No reporter tombstone, save, ROM or imported media is committed. Reporter
confirmation and a new APK are required before claiming this fixes the Moto.

Sibling font-allocation failure in FontWithCap is a separate defect/PR. Neither
the atlas finding nor that allocation test demonstrates the cause of Deck330.
