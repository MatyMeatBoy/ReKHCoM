# Card pixel corrections: current coverage

The requested change is literal: remove the stray white seam without retouching
the approved illustration. Match each card's own GBA palette, rather than making
every top line black.

| Report | Evidence / action | Remaining verification |
| --- | --- | --- |
| Kingdom Key / red attack card white point | Native cell11,15 is white; neighbours are RGB8,8,8. The standard128x256 HD loader covers only that8x8 cell with opaque RGB8,8,8. All other32704 texels are unchanged. Windows32 EU D3D11 before/after deck captures reached the same view with12 cards and normal values0..7. | Additional premium combinations and platform gameplay checks. |
| Kingdom Key values0..9 | Actual card catalog maps all ten values to the same small-card artwork; the loader correction therefore applies to each. | This is source mapping, not ten gameplay captures. Digits remain separately rendered. |
| Heroica / Olympia5 right-edge white points | PR405 corrects native cell14,19 only when HD queues successfully; the legitimate HD highlight stays unchanged. Layered SDL reproduces it; native32 EU D3D11 with the user's copied L18 save shows values4/6/5 and five corrected residue pixels beside the separate Kingdom Key fixes. | Additional platform gameplay checks. |
| Frame-closing corners | PR408 keeps the original cells1,16 and14,16 above the HD illustration, in each card's own palette. Includes front-card ownership so a lower corner cannot cover a higher HD card. | World/screen game capture and software/HUD branch tests passed; other devices remain separate. |
| Attack-card horizontal seam | The audit of18 attack illustrations found23 old-art cells in row15 across nine cards; see list below. Only accepted HD replaces those cells with their original neighbouring black. | Actual game comparison of this scoped extension is pending. |
| Cure top line | Decoded GBA palette is black RGB0,0,0; preserved. | No blanket recolouring. |
| Simba / Potion top line | GBA Simba is blue RGB0,57,189; Potion is green RGB0,139,0; preserved. | Their own closing corners retain these colours. |

Horizontal old-art residue, native cells in row15:

- Crabclaw: x9,10; Divine Rose: x8,9,10; Fairy Harp: x11,12.
- Metal Chocobo: x11; Oathkeeper: x10,13; Pumpkinhead: x8,9,10,11.
- Spellbinder: x8,9,10,11; Ultima Weapon: x10; Wishing Star: x8,9,10,11.

Only those23 cells change. Kingdom Key retains its existing one-cell correction.
Diamond Dust and One-Winged Angel keep their native RGB16,24,32 seam, and Soul
Eater keeps RGB8,8,8. No common black recolour is applied to other cards or to
the rest of these nine illustrations. The helper runs after transparent-pixel
rejection and before the existing blend/fade processing, in both native routes.
Coordinates are authored frame coordinates, so partial/flipped/affine OAM pieces
continue to follow the same cell. Missing/rejected/disabled HD stays native.

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
