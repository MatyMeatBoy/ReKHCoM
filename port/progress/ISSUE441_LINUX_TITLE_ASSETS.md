# Linux title resource packaging (#441)

The public v2.2 Linux desktop/Deck tar (SHA256 dc8a5d0b138debf282088629ff05b234c663faacc56063337290b0761f27eb75) contains no assets/hd/title resources. The Windows ZIP contains 126. The relevant PPU, HD sprite and presenter sources are identical between the recorded Windows game source 38f84bd and Linux ace4116. Windows can also load embedded resources; the non-Windows PortEmbeddedAssetData implementation returns NULL.

Linux bundle() copied fonts, badges and gallery files but omitted the existing shared core_assets catalog. With hd_title enabled, the gradient was still presented above the cartridge picture after the unavailable HD words fell back to native sprites. Its left edge covered the final English T. This is a resource availability difference, not evidence of a Linux-specific font or GPU defect.

The packaging change copies the existing 335-entry core catalog, including the 126 title resources and other shared runtime assets, into i386/desktop and armhf bundles. It changes neither image bytes nor game code, emulation, timing, compositor order or user settings. The published v2.2 artifacts remain unchanged. A defensive compositor change for rejected/missing resources is a separate follow-up; this packaging fix does not claim to make such a fallback safe.

## Verification

- Real bundle() resource consumer test, with only native library scanning/strip mocked: before the fix both architectures omit 135 catalog entries; after it both contain all 335 entries with exact source hashes. No game or ROM executes in this packaging unit test.
- Exact public Linux 32-bit executable SHA256 cfbf65c9be194e2a6ed93db6abfc0bc60536d64c12f07aee0a7eee16046b3b7e is unchanged in all nine VM cases. A private EU cartridge copy is used without distributing it. At 960x720, view1, English, frames600/660/720: the unmodified tar reproduces the cut; adding only the five English Press Start frame PNGs restores the letter without adding the HD poster. At pixel450,620 on frame660, the missing-resource case is pink (222,165,181), while the words-present case is black (0,0,0).
- The complete shared catalog is consumed by the actual title renderer in English, French, German, Spanish and Italian. Exact `hd draw` acceptance records are retained. Normal hd_title=0 controls with/without the catalog are pixel-identical at all three captured frames.
- Windows exact v2.2 EU32, actual D3D11 and SDL software, same region/language/view/window: the full letter is visible with the packaged resources. Earlier private shots taken before title settlement or with a cropped auto view are retained as unsuitable controls, not used as the matched comparison.
- Linux VM: cached Debian snapshot,768MiB/one CPU/no network, native SDL software using a dummy video surface with real rendered output capture. No Xvfb/Mesa/GL cache is available. This is not physical Deck/GamingMode or Linux GPU certification; Windows D3D11 is the actual GPU control.

Evidence: pruebas/issue441-title-start-bar/ at the workspace root, including the original report image, EXACT-PACKAGE-TITLE-ASSETS.json, PACKAGING-RED/GREEN.txt, vm-run/evidence, PIXEL-CHECKS.json and Windows capture receipts. The raw VM receipt initially used a broad name substring which also matched rejected sprite logs; PIXEL-CHECKS.json corrects consumption using actual `hd draw` records (the missing case has zero accepted words). No proprietary input is included in this PR.

The final new test archive still needs its own member/hash and affected-flow verification before delivery. Local regressions do not constitute the reporter's confirmation. Reference: https://github.com/MatyMeatBoy/ReKHCoM/issues/441 .
