# Área medida del medidor HD y límite del runtime de 64 bits

Carril `hd-gauge-visible-bounds`, owner `codex-gauge-bounds`, rama
`fix/hd-gauge-visible-bounds`. Reporte del usuario: bordes internos dentados.

## Cambio nativo con presupuesto conservado

El rectángulo temporal del medidor era 680×280 unidades de layout, aunque
la geometría visible ocupa mucho menos. Se mide con `DrawSprite` y
`PortHdLayoutDraw` C reales: el renderer CPU registra los vértices emitidos
después del recorte de máscaras, incluyendo las transformaciones y la
interpolación reales. No se reconstruyen las curvas mediante una fórmula
inventada ni se distribuyen archivos importados.

Se recorrieron todos los cuadros enteros 0..470 de los grupos 7/8/9, todos los
cuadros de los estados usados 10/13/14/15 y las cuatro caras de la secuencia 0.
Sora y Riku usan esa geometría facial con distintas texturas.

| Importación | Límites X emitidos | Límites Y emitidos |
|---|---|---|
| HD remasterizado | −89,521…202,010 | −89,521…89,521 |
| Layout anterior | −89,521…203,010 | −89,521…89,521 |

El nuevo rectángulo constante es **[−92, −92, 206, 92]**, 298×184 unidades,
con margen alrededor de toda la geometría emitida. No cambia por HP, máximo,
estado o cara: esos cambios no causan asignaciones de texturas. El rectángulo
viejo empezaba en −60 y ya cortaba parte de la geometría del resplandor de
poca vida. El nuevo incluye ese resplandor.

Se mantienen exactamente los límites de 32 MiB desktop y 12 MiB limitado,
límites del renderer, fallback SS4→SS2→directo, propiedad de las texturas y
liberación antes de redimensionar. La geometría directa sigue disponible si
no cabe ninguna cadena. No se toca HP, daño, curación ni la lógica de combate.

| Escala del juego | Tamaño base redondeado | Ruta desktop, memoria RGBA de la cadena |
|---|---|---|
| 6,75 (alto 1080/160) | 755×466 | SS4: 29.553.720 bytes, 28,19 MiB |
| 9 (alto 1440/160) | 1006×621 | SS2: 12.494.520 bytes, 11,92 MiB |
| 13,5 (alto 2160/160) | 1509×932 | SS2: 28.127.760 bytes, 26,82 MiB |

Son superficies temporales contadas, no memoria GPU total ni una medición de
FPS. La ruta limitada mantiene SS2 en 1080/1440 y cae a directo en 2160 al
superar sus 12 MiB. No se aumenta el gasto permitido para conseguir el cambio.

## Regresión y procedencia

- `test_hd_gauge_visible_bounds.py --lad RUTA` compila las funciones C reales
  y verifica que todos los vértices emitidos caben en el rectángulo productivo.
  Puede repetirse `--lad`; sin imports declara SKIP, no una validación visual.
  Antes del cambio detecta el resplandor cortado; después pasan ambos imports.
- `test_hd_gauge_targets.py` usa las funciones reales de dibujo/caché con
  asignaciones contadas: 1080/1440/2160, 1.000 cambios de HP/estado/cara sin
  nuevas asignaciones, límites, fallos parciales/totales, resize y shutdown,
  bajo perfiles desktop y limitado. Las expectativas de calidad fallan con el
  rectángulo anterior y pasan con el medido.
- SHA256 de import HD local: `121dc7c15b3494bba21a9b21d2f92d6ffac60e40a84200b00739be9ee7ff173a`.
- SHA256 de layout anterior local: `89bfa78d80b7e013ffe82534410847bdd95e06a9345bda2a117b2d486228cb87`.

La medición incluye geometría con texels transparentes, por lo que sobreestima
conservadoramente el contenido visible. Una importación distinta debe volver
a medirse. Faltan capturas GPU, baja vida, daño y aparecer a varias escalas;
CPU no demuestra por sí solo la suavidad de los píxeles finales.

## Windows 64 y guests: pendiente distinto

**Este cambio no habilita supersampling en Windows 64.** El guest se construye
con `-ffreestanding` mediante la base `port/ps5/build_ps5.py`; su
`__STDC_HOSTED__` es falso. La cadena de targets en `port_hdlayout.c` está
bajo `#if __STDC_HOSTED__` y el shim SDL actual no declara
`SDL_SetRenderTarget` ni `SDL_TEXTUREACCESS_TARGET`. Su ruta sigue dibujando
la geometría directa. Por ello no se afirma que este PR corrija la captura
dentada reportada desde Windows 64.

Siguiente paso: medir el camino de dibujo del host y su bridge; evaluar AA en
host o una extensión opcional de render targets sin romper el ABI HF existente
ni exigir targets a consolas/hosts antiguos. Probar host nuevo/guest nuevo y
combinaciones compatibles, fallback sin capacidad, renderer software y GPU,
presupuesto real y restauración de viewport/clip. No reemplazar esa prueba
por aumentar el presupuesto global del guest o activar APIs ausentes.

La equivalencia del arco 155/155 y el fallback normalizado sigue registrada en
[HUD_ENEMY_STATIC_TRACK.md](HUD_ENEMY_STATIC_TRACK.md); este recorte no cambia
esas fórmulas ni demuestra paridad exacta con una ejecución de Re:CoM 1.5.
