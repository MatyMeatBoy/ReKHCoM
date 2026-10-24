# Rendimiento (21/10/2026)

Pedido del usuario: el máximo rendimiento en todas las plataformas, preciso y sin bugs; en Windows, zoom x1 a
velocidad completa con margen.

## Qué dibuja la CPU y qué la GPU

| Plataforma | Hoy | Plan |
|---|---|---|
| Windows | GPU (`direct3d11`) para presentar; la CPU arma la imagen GBA (PPU) | PPU en varios núcleos (abajo) |
| Linux / Steam Deck / Anbernic, Android 32/64, web, PS5, Switch (desde pre2) | GPU | — |
| PS Vita | CPU: rasterizador por software a 960x544 | GPU por SDL2 (GXM), el mismo código que Switch; el VitaSDK de la VM no trae SDL2: hace falta el paquete oficial (`vdpm sdl2`), pedir OK para descargarlo |
| PS4 | CPU: rasterizador por software a 1080p | GPU por Piglet (OpenGL ES de la PS4); el SDL2 de OpenOrbis no tiene renderer por GPU |

## Medido en Windows (1920x1080, ventana oculta con D3D11, `KHCOM_HIDDEN=1`, `build/perf.sh`)

Tiempo de trabajo por cuadro (lógica + PPU + presentar); presupuesto 16,74 ms. Medido con las baterías corriendo
a la vez (2 procesos), así que los picos están inflados.

| Escena | x4 (480x270) | x1 (1920x1080) antes | x1 con PPU en 8 núcleos |
|---|---|---|---|
| Combate | 3,2 ms | 8,0 ms | — |
| Sala (campo) | 5,1 ms | 44 ms (PPU 36,5) | 23,7 ms (PPU 17,4) |
| Escena de evento | 6,9 ms | 59 ms (PPU 61) | pendiente |

A x1 en campo y escenas la PPU es el cuello de botella; la GPU tarda ~4 ms.

## PPU en varios núcleos (`port_ppu.c`, `RenderOuterRows`; zona de Codex, tomado por pedido del usuario)

- Las filas del lienzo por encima y por debajo de la imagen GBA muestran el estado de la línea 0 o de la 159: entre
  ellas no corre ninguna interrupción ni DMA del juego, así que se reparten entre núcleos. A x1 son 920 de 1080.
- Cada núcleo tiene sus búferes de línea (`PPU_TLS` = `_Thread_local`; TLS nativo en llvm-mingw, sin coste en los
  bucles). Solo con `PORT_PPU_THREADS` (configure_pc.py, Windows); consolas, web y Android sin cambios.
- Igual píxel a píxel: cada fila recibe el punto de referencia afín que tendría; los glifos de texto HD encontrados
  se juntan en el orden de filas; los cuadros con estado entre filas siguen en un núcleo (mosaico vertical, suelo
  isométrico, cabeceras de menú). Las tablas por sala de `MapWorldTile` se llenan antes (`MapWorldTilePrime`).
- `KHCOM_PPU_VERIFY=1` vuelve a dibujar cada tanda en un núcleo y anota `ppu verify: MISMATCH` si algo difiere;
  `KHCOM_PPU_THREADS=n` fija los núcleos (1 = uno).
- **Regla para quien toque la PPU:** todo búfer que una función de línea escriba debe ser `PPU_TLS` o indexado por
  fila; las cachés que dependen de cuadros anteriores van en `PpuRowsIndependent`.

### 22/10: suelo isométrico (escenas aprobadas 004, 057, 061, 074...)

`perf_x1.py` (1920x1080, D3D11 oculta, tiempo real, baterías corriendo a la vez): el evento 004 a x1 no llegaba a
5 fps (más de 200 ms por cuadro: 1500 cuadros no terminaban en 5 minutos). Dos causas en `port_ppu.c`:
- Una sola tabla `sIso` para todas las capas: con dos capas isométricas de mapas distintos, cada consulta de una
  borraba la de la otra y se recontaban 4096 tiles **por píxel**. Ahora una tabla por fondo (`sIsoTables[4]`),
  llenada por `IsoPrime` antes de repartir las filas: el suelo isométrico ya se dibuja en varios núcleos.
- `IsoEntry` caminaba la diagonal hasta el mapa en cada píxel: ahora recuerda por tile la coordenada de destino
  (no el valor: el mapa se lee en vivo y los tiles animados siguen moviéndose).
Resultado: evento 004 a x1 de >200 ms a 31 ms por cuadro (PPU 121 → 28 ms). Píxel a píxel idéntico al build
anterior en 004/016/057/061/074 (4 capturas cada uno) y 0 `MISMATCH` con `KHCOM_PPU_VERIFY` (`compare_ppu_builds.py`).
El perfil (`KHCOM_PROFILE`) ahora dice cuántas tandas de filas van en varios núcleos y por qué otras no.

Medido después (x1): campo 18,8 ms (PPU 14,7), evento 016 45 ms (PPU 37), evento 004 31 ms, combate ~25 ms.
Lo que queda: las 160 filas de la imagen GBA se dibujan en un núcleo a todo el ancho (1920 px) porque entre ellas
corren los eventos de línea del juego (HBlank, DMA). Repartirlas pide guardar por línea el estado que usa la PPU
(registros, paleta) y dibujar después: es el siguiente paso para 60 fps a x1, no entra en la v2.0.

Falta: verificar con `KHCOM_PPU_VERIFY` en toda la batería de eventos/salas/combates (0 diferencias), bajar la parte
secuencial (las 160 filas de la imagen GBA y la presentación) para llegar a 60 fps con margen a x1, y Linux.

## Salto de cuadros (`experimental_frame_skip`, 21/10/2026, issues #35/#36)

Un cuadro que empieza medio período tarde no se dibuja ni se presenta (máximo tres seguidos): la lógica corre una
actualización por VBlank con todos sus eventos de línea (VCount, HBlank, DMA de HBlank: `PortPpuRunFrameEvents`), así
que el juego y el sonido mantienen la velocidad original en equipos que no llegan a dibujar cada cuadro (Switch,
Vita). Encendido por defecto en todas las plataformas por pedido del usuario, tras comprobar:

- `test_frameskip.py` (`KHCOM_SKIPTEST=1` salta 2 de cada 3 cuadros a la fuerza): 24/24 eventos con las 4 capturas
  idénticas píxel a píxel a la corrida normal. Está en la batería (`desktop skip eu/us/jp`).
- Windows en tiempo real con VSync a 60 Hz y las baterías corriendo: 1 cuadro saltado de 3600.
- El log dice cuánto actúa: `frame skip: N of the last 600 frames not drawn`.
