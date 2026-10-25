# Traspaso activo — cargas, integración y ZIP local (25/10/2026)

## Encargo vigente

El usuario pide resolver los pendientes técnicos, evitar tirones preparando el
trabajo durante cargas/fundidos y evaluar cachés pequeñas en RAM/GPU. Claude
procesará el arte pendiente. Codex integra las versiones aprobadas, termina de
integrar/cerrar todos los PR y crea un ZIP local con las soluciones. El arte
aprobado debe entrar antes del ZIP. No publicar una release ni comentarios en
issues por inferencia. Pushear cada avance y actualizar este traspaso.

**Cuidar RAM: juega Deadlock.** Sin compilaciones ni pruebas paralelas, Ninja
`-j 1`, pruebas ocultas/silenciosas, `KHCOM_PPU_THREADS=1` y
`KHCOM_BACKGROUND_TEST=1`. La RAM libre fluctúa entre 0,5 y 1,5 GB. El emulador
Android abandonado fue cerrado para recuperar unos 977 MB; no volver a abrirlo
durante presión de memoria. No tocar Steam ni Deadlock.

## Hecho y subido

- PR #91 integrado: Jafar ya no pierde BG0 entre cuadros al desplazarse; el
  escenario mantiene su efecto y el genio tiene una sola copia. 1800 cuadros
  16:9 y 560 cuadros 32:9 sin pérdidas, centro GBA idéntico. Aprobación Atlas
  `e182f7052b8348b2a88b4f2e996a0ed7`. Detalle: JAFAR_BLINK_20261025.md.
- PR #97 integrado: verificación Steam, 4/4 escenarios sintéticos y cuenta real
  correctos. La sospecha de parser era falsa; no se modifica Steam.
- PR #98 integrado: el CRC opcional JP de 32 MB se verificaba al entrar al
  título (125 ms). Ahora CheckNewAssets corre antes de AgbMain; transición con
  JP medida a 16,74 ms. `test_title_startup.py --jp-rom ... --gpu` y
  `check_sources.py` correctos. Incluye opt-out de prioridad para pruebas.
- PR #92 integrado: escena 102, versión `dd5a82c201a84ca29180026bc7cf064a`.
- PR #93 integrado: escena 115, versión `edd1e953dd7141ad931a43429c621a42`.
- PR #94 integrado: escena 116, versión `4767d7d89689440084a12cf7beeab8b9`.
- PR #95: escena 117 lista para integrar, versión
  `ff9f8d6e16a149708fb8a08c5c80d431`. Rebase conserva 102/115/116;
  `test_preliminar_v1.py --approved --quick --only escena-117` pasa.
  Su rama se está pusheando junto a este traspaso.

Las escenas aprobadas se habilitan en `kPreliminarApproved` y se prueban sin el
flag de preview, comparadas con su interruptor `=0`: centro GBA idéntico.
No cambiar las valoraciones ni declarar resueltas las nuevas nubes. El usuario
autorizó explícitamente estas versiones aprobadas antes del ZIP; la excepción
puntual está en REGLAS_DEL_USUARIO.md y LOGICA_APROBADOS.md.

## Siguiente, en orden

1. Terminar #95 y #96 (118, versión `09dcd37573df49b88edbd71f79e06b82`).
   Resolver conflictos preservando las recetas de todas las otras escenas.
   117/118 agregan casos separados en PixelSource y en el camino escalar PPU.
   115/116 tienen casos separados de TileSource. No descartar una al resolver
   la otra. Actualizar cuerpos de PR para reflejar la activación aprobada.
2. Integrar #90: sólo versión v2.0-pre13, recursos Windows y changelog; ampliar
   notas con Jafar, título y aprobados. No crear release pública.
3. Rama `codex/transiciones-cargas` y reserva `transiciones-cargas`, propietario
   `codex-transiciones`: auditar y corregir primeras cargas en cuadros visibles.
4. #83 Android sigue **sin corrección confirmada**. Prueba PC Sora 4000 cuadros:
   10 drops, 1 recogida, salida 0, sin BIOS/CRASH. No demuestra solución Android.
   Verificar Riku y usar la reproducción real cuando la RAM lo permita.
5. Compilar EU/US/JP serialmente, verificar fuente/versión/binarios y ZIP; crear
   paquete local nuevo sin ROMs. No reemplazar ni borrar paquetes jugar/vX.

## Auditoría de cargas: hallazgos, todavía sin implementar

- Ya existe lector asíncrono de OGG (`port_hdmusic.c`, un hilo y una pista).
  Evitar precargar toda la música. Revisar apertura/decodificación y .loop,
  que todavía se resuelven en el hilo del juego al terminar la lectura.
- `PortHdSpriteFind` verifica archivos en su primer uso; `Load` decodifica y
  sube texturas desde `PortHdSpritesDraw`. Caché LRU existente de pares normal/
  blanco: presupuesto 32 MB desktop, 12 MB consolas. Su límite puede excederse
  si todos los candidatos cuentan como usados en el mismo cuadro. Auditar.
- `PortHdLayoutReady` abre archivos mientras no hay textura; los LAD y las
  texturas del HUD se cargan al primer uso. Evitar esas pruebas repetidas.
- `PortHdHudPrepare` se llama desde Present: primera carga PNG + validación +
  máscara blanca + label. Reubicar preparación inicial, conservar caché.
- HD text registra fuentes GDI y rasteriza glifos al primer uso. Cachea 2
  escalas por estilo, pero necesita precalentamiento razonable.
- Las insignias del primer logro se leen y decodifican desde Badge durante el
  dibujo del toast. Su atlas es pequeño y puede precalentarse/compactarse.
- `PortAssetPresent` caduca cada segundo; la comprobación general ya se ejecuta
  al arranque, pero auditar llamadas desde menús/cambio de modo.
- Vídeos HD: lectura y decoder en PlayFile, invocado entre modos; revisar qué
  cuadro queda presentado al bloquear. No almacenar películas completas.
- SRAM sólo escribe al detectar cambios (comparación de 32 KB); preservar
  seguridad y cambio de banco. No cambiar a async sin pruebas de integridad.

Inventario real (PNG headers, sin decodificar): todos los sprites únicos
representan unos **247 MB en pares GPU**, frente a unos 16 MB comprimidos.
Grupos: cartas 68,73 MB; título 20,09; HUD 22,27; mundos 40,74; rótulos 68,71;
cartas pequeñas 26,25. Precargar indiscriminadamente es caro; seleccionar los
recursos de la escena/deck y mantener una caché acotada. Medir frío/caliente,
cuadros visibles y fases cubiertas; no sólo FPS promedio.

## Entorno y archivos locales

Trabajar explícitamente en `KHCoMPC/khcom-jafar`, sin tocar WIP de otros árboles.
El build EU incremental es `build/pc-eu`; el exe debe recompilarse al integrar.
ROM EU local ignorada: `roms/B8CP.gba`. US/JP están en `../roms` (KHCoMPC/roms; las carpetas de
`jugar/` viejas se borraron el 25/10). HD privado: `D:/01Claude/ReKHCoM-build/hd-completo`. No empaquetarlo.

Atlas compartido: `../khcom-codex/build/art-review`; servidor localhost:8766,
no detenerlo. Sus comentarios/aprobaciones se leen desde Store, no SQL directo.
Ayudas locales ignoradas en `build/`: `accept_art.py`, `resolve_art.py`,
`art_capture.py`, `hd_inventory.py`, `test_gimmick_desktop.py`; capturas y
reportes de aprobados en `build/approvedNNN`. Reproducción Jafar y pruebas de
título están versionadas. `build/pc-eu/khcom.exe` actual incluye hasta 117.

Reserva de integrador: `codex-integrador`, activa. Token sólo en el archivo
local ignorado `build/integrator-token.txt`; tocar antes de integrar y liberar
al terminar. La cola remota aún necesita actualizar 91/92–96 como integrados y
separar el patrón del suelo Jafar, que queda para Claude.
