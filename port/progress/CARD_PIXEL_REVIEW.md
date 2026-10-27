# Card pixel corrections: current coverage

The requested change is literal: remove the stray white seam without retouching
the approved illustration. Match each card's own GBA palette, rather than making
every top line black.

| Report | Evidence / action | Remaining verification |
| --- | --- | --- |
| Kingdom Key / red attack card white point | Native cell11,15 is white; neighbours are RGB8,8,8. The standard128x256 HD loader covers only that8x8 cell with opaque RGB8,8,8. All other32704 texels are unchanged. Windows32 EU D3D11 before/after deck captures reached the same view with12 cards and normal values0..7. | Additional premium combinations and platform gameplay checks. |
| Kingdom Key values0..9 | Actual card catalog maps all ten values to the same small-card artwork; the loader correction therefore applies to each. | This is source mapping, not ten gameplay captures. Digits remain separately rendered. |
| Heroica / Olympia5 right-edge white points | Native white cell14,19 and the HD window boundary require a layered rendering reproduction. No speculative dark patch to the legitimate illustration highlight is included. | Reproduce the reported edge at runtime, correct only exposed residue, then check all values. |
| Cure top line | Decoded GBA palette is black RGB0,0,0. | Compare current runtime edge against that source; no blanket recolouring. |
| Simba top line | Decoded GBA palette is blue RGB0,57,189. | Preserve this blue; do not apply Cure's black to Simba. |

The production-loader regression captures the uploaded pixels and checks every
texel, including unchanged Olympia/Cure/Simba and an unrelated group. It uses a
synthetic decoded image; it does not establish rendered filtering quality.
Budget, first-frame fallback and stock-card-frame contracts are checked separately.
Missing/disabled HD retains the original GBA rendering. Other image sizes are
unchanged. The shared loader correction applies wherever this HD sprite backend
is built; platform gameplay checks remain separate.

The current-source before/after game check compares the exact published pre2
EU native executable with this correction, identical ROM and imported sprite
fixture, at frames300/400/460. Both runs exit normally and log the actual HD
Kingdom Key and value-digit consumers. The fixture is a partial legacy import,
not proof of all current HD families. No proprietary images are redistributed.

Keep this ledger open until the remaining rows have direct evidence. Earlier
experimental Heroica previews were rejected and are not a delivered correction.
