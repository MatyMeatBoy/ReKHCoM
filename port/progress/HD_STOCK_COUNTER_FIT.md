# Encaje del total de stocks con HP HD

Claim `hd-stock-counter-fit`, rama hija de `qa/v21-integrated`170002a66.
Capturas root: `build/v21-final-stock/stock-axel/host_shot_420.bmp` y
`stock-player/host_shot_650.bmp`. El total blanco queda detrás de la barra HD;
los marcos ya se ven. Config real: `hd_cards=1`, `hd_hud=0`, HUD escala5.

Caller exacto en ambos `cardbattle_2` (`card_battle.c`, `card_battle_riku.c`):
`DrawSprite(work->x,4,gStockValueFrames[0],...)` con suma1..27, tiles actualizados
con `gStockValueTiles+(sum-1)*128`. No son los dígitos individuales de cartas.
El frame es16x16. ROM EU propia, sin bytes incluidos: bbox opaco local
[0,4,16,15), blanco y[5,11) (sumas1,12,17,19,27). Anchor4 produce tinta45..75
con escala5; la barra jugador termina cerca de y60. Anchor9 da placa65..120,
tinta70..100: separación5px bajo la barra. No se cambia X, cartas ni artes.

El cambio se aplica sólo en el render HUD PPU para esos27 sources alineados,
si `PortHdStockValueTop` confirma que el gauge importado del lado está activo
en el cuadro actual. La composición nativa/fallback conserva anchor4 y no se
modifica OAM ni lógica de batalla. La identidad y función de render existentes
conservan las rutas de dibujo; no hay cambio global de orden ni configuración.

`test_hd_stock_counter_fit.py`: callsite real de coordenadas, política activa
real del layout, RED exit77→GREEN ambos lados/sumas1..27, fuentes distintas,
missing/stale/off gauge y valores ya inferiores preservados. Syntax-only x32
PPU/layout y fullPPU/preflight pasan. Sin juego/build pesado. Root recaptura
Sora/Axel de inmediato y verifica bbox/ausencia de cruce con dígitos de cartas.
