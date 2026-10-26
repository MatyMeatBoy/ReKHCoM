# Recarga HD: componer el contador sobre el relleno original

Pedido del 26/10/2026: la carta de recarga del tráiler aparecía roja y vacía.
Se reprodujo con la misma entrada de Jafar: `KHCOM_BATTLE=204:8:0`,
`KHCOM_MONKEY=19:120`, `KHCOM_KEYS=60-63:A`, ventana 1920×1080,
view 4, zoom 8, `card_hud_raise=8`, cartas HD activadas y piloto `hd_hud=0`.
En el cuadro 1572, el estado real tiene gauge 256 y contador interno 0 (glifo 1).

## Causa y cambio

La recarga no es una sola imagen. `card_reload_2` en `src/card/card_battle.c`
y `Reload_Card_2` en `card_battle_riku.c` dibujan dorso, relleno, remate y contador
con prioridades distintas. `RELOAD_2`/`RELOAD_CHILDREN_2` son efectos de cartas,
no el medidor activo; tampoco se debe confundir con `REV_COUNT`, el tamaño del mazo.

El port encolaba tanto el dorso como el número como sprites HD y luego presentaba
una capa con los sprites nativos que debían estar delante. El relleno y su remate
nativos quedaban por encima de **todos** los sprites HD, incluido el contador que
el original pone delante. El número desaparecía bajo el rojo o el blanco del relleno.

La familia de diecinueve cuadros `gReloadCounterWhiteTiles_*` se presenta ahora
sobre esa capa nativa. Conserva el quad, los UV, la opacidad y la paleta del cuadro
que eligió el juego. Una máscara de propiedad final oculta el número donde hay
objetos realmente delante, comparando prioridad OBJ primero y orden OAM después;
permite que su tinta HD use toda su celda sobre objetos de fondo, sin restringirla
a la silueta pixelada del glifo GBA. La propiedad se registra también cuando solo
están activadas las cartas HD, sin requerir el piloto de HUD.

No se cambia ningún asset, tarea, contador, tiempo, sonido, animación, matriz ni
cálculo de progreso del juego. El relleno sigue siendo exactamente sus tiles,
paleta y matriz afín; no se añade una barra pintada ni un temporizador paralelo.
Las colas se reinician cada cuadro, de modo que no persiste un número de una tarea
que ya se cerró. Si el contador HD no está importado, conserva el dibujo GBA.

## Contrato verificado contra la decompilación

- El borde inferior del relleno se mantiene en `y+17`; la matriz comprime sus
  cuadros 32×32 desde abajo. Gauge cero no dibuja relleno. A 256, `AllocObjAffine`
  devuelve NULL para identidad y el relleno sigue presente.
- El remate se muestra solo con gauge positivo y fullFlag. Su secuencia de estado
  pasa 4→7→10→13→16→19→22; el mismo update de 22 restablece 4, baja el contador
  y reinicia gauge/fullFlag. No se fabrica una imagen visible adicional de 22.
- Soltar A conserva el progreso parcial y selecciona los cuadros idle originales.
- Los ciclos iniciales 0/1/2 corresponden a las cuentas visibles 1/2/3; se consumen
  una, dos o tres cargas. El contador final vuelve a estar limitado a 2 (visible 3).
- La ruta `Reload_Card_2` es la del oponente humano/VS. Su escala horizontal afecta
  también base, contador y remate, y no dibuja con escala <=0. El jugador de Reverse
  Rebirth utiliza la recarga instantánea y omite este contador por diseño original.

## Pruebas y evidencia

1. `test_hd_reload_order.py`: RED sobre el cuadro real 1572 anterior (0 píxeles
   blancos interiores), GREEN después (1342). La diferencia completa se limita
   a `(90,860)-(145,940)`, la tinta del número; relleno, marco, corona y mundo
   fuera de ese rectángulo quedan idénticos. El arnés también reproduce la entrada
   EU con `--exe --rom --sprites --work`, en un directorio privado, sin partidas.
2. `test_hd_reload_mask.py`: ejecuta el nuevo pase y el predicado PPU reales;
   relleno detrás, objeto delante, oclusión parcial, prioridades OBJ distintas,
   zoom, recorte, alfa, paleta/fundido blanco, ausencia de recurso y reinicio de cola.
   El caso de prioridad mixta falla con el predicado anterior y pasa con el actual.
3. `test_hd_reload_decomp.py`, preparado por el revisor independiente: ejecuta los
   updates/draws originales Sora/oponente Riku con servicios de animación/audio/OAM
   aislados. Gauge 0/25/128/250/256, identidad NULL, remate, ciclos 1/2/3, OVERDRIVE,
   retención parcial, bob/diálogo, escala Riku 0/64/128/192/256, cuadros 1..19 y
   ausencia con GFX no cargados. Sin cambios a estas funciones de producción.
4. HD apagado: ocho cuadros antes/después de carga, remate, flash y reparto,
   píxel por píxel idénticos. Contadores HD ausentes (solo dorso HD disponible):
   otros ocho cuadros idénticos, conservando el contador GBA.
5. Renderer nativo x86 EU/US/JP compilado. Runtime con ROM US y JP propias:
   contador HD visible sobre relleno parcial en ambos, 408 píxeles blancos en el
   ROI de control; se conservan capturas 530/540/560. Todas las pruebas SDL usan
   drivers de vídeo/audio `dummy`.
6. El productor ejecutó tres ciclos reales con ataques, selección y A, sin
   escribir memoria ni usar savestates: gauge 100 retenido tras soltar A en los
   tres, full 256 y cuentas 1→2→3. El trace de solo lectura incluye la secuencia
   interna completa y el tope final original. Es control QA, no estado pintado.
7. `check_sources.py` compila contra el shim común de consolas. No se afirma
   prueba en hardware físico. El WIP ajeno de `khcom` se leyó como referencia y
   no se modificó. No se generaron assets nuevos ni se publicó release.

Capturas y controles locales: `build/reload-probe`, `build/reload-after`,
`build/reload-matrix`, `build/reload-missing-matrix`, `build/reload-regions` y
`build/reload-runtime-final`. El trace del productor está bajo la carpeta pesada
`<work>/trailer-v20/capture/reload_probe_three_cycles/reload_trace.jsonl`.
