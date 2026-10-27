# Pendientes después de v2.0 — 26/10/2026

## Mandato vigente de coordinación — 27/10/2026

Consultar [COORDINACION_OPTIPORT_V21.md](COORDINACION_OPTIPORT_V21.md) antes de
tomar trabajo: **Codex termina OptiPort al 100 %, prerelease Windows 64 y ZIP
personal limpio; Claude puede tomar todos los pendientes regulares** (HUD,
jefes, recetas, Atlas, galería, etc.), reservando en `coordination/art-queue`,
preservando WIP y abriendo PR. **Última actualización del usuario:** Codex recoge
e integra los PR regulares ya listos antes del ZIP Windows, con revisión y lock;
entrega el ZIP prioritario y después completa el resto de OptiPort. Los demás PR
regulares se integran después de OptiPort. Los claims activos de core/guardas, arquitectura y QA se respetan;
el trabajo de Claude ya integrado sigue vigente.

Esta lista es un snapshot, no autoridad de reservas. Registrar por ID owner,
rama, scope/rutas, dependencias, reproducción/evidencia, SHA/tests, PR/merge,
artefacto y siguiente prueba mínima. Integrado ≠ publicado ≠ confirmado en el
dispositivo. La primera pasada original 9/14 (~64 %, aviso del usuario) es
histórica; migración 32→64 es alcance añadido con seguimiento separado y no
un porcentaje de estabilidad.

**Autorización actual:** prerelease y envío de builds a #277/#272, más ZIP
personal normal de Windows sin etiqueta alpha. La final 2.1 requiere completar
OptiPort; no se promete cero bugs. Windows: Auto/Forzar 32/Forzar 64; Linux
desktop/Deck: un paquete 64/32; Android: APK universal 32+64; consolas beta.
Verificar SRAM manual/config entre runtimes y namespaces/compatibilidad de
quicksaves. La instalación Quiver principal sigue protegida, sin pruebas allí.

- [ ] **Vista/previews en ambos modos:** identificar primero los dos modos en
  el UI actual, sin asumir cuáles son. Pendiente posterior al ZIP prioritario:
  OptiPort mide entrada, p95, memoria, caché, precarga/cancelación y calidad;
  Claude toma el fallback visual y presentación como tarea regular bajo claim.
  Objetivo: carga casi instantánea, poca pérdida de calidad y evitar esperas
  largas al entrar a escenas. Sin implementación ni garantía medida aún; ver
  [coordinación](COORDINACION_OPTIPORT_V21.md#pendiente-vista-y-previews-en-ambos-modos).

- [ ] **Cierre posterior a 2.1: ordenar worktrees** según el documento de
  coordinación, conservando WIP, partidas, ROMs, evidencia y commits. Ahora
  solo inventario/registro; no mover activos ni borrar. Nuevos worktrees:
  preferir `KHCoMPC/khcom-worktrees` según la raíz del repositorio.

- [ ] **Galería: retorno directo al terminar/saltar escena**, sin destello
  intermedio de título/gameplay. Claude regular UI, dependencia de pacing
  OptiPort si aplica. Probar título/partida × normal/skip × GBA/ReKHCoM,
  capturando cuadros, restaurando UI/renderer/input/audio sin avanzar historia.
- [ ] **Prompts de mando completos, experimental 2.1:** Auto, Original/GBA,
  Nintendo, Xbox y PlayStation; bindings efectivos coherentes con tutorial,
  convención Nintendo A/PS círculo elegible, assets existentes, todos los
  botones GBA de Opciones, overrides, hotplug y localización. Solo pendiente.
- [ ] **Gate real del ZIP Windows 64:** primera puerta con guardado del usuario
  confirmada RED por coordinador, WER `0x100f7bd6`,
  `MapWallFaceIsUnreserved`, fila superior NULL/lectura BIOS GBA. Coordinador
  corrige fuente compartida; exigir GREEN del mismo repro en build final antes
  de entregar ZIP. Las ampliaciones UI anteriores no retrasan esa entrega.

  Actualización del coordinador: repro corregido GREEN, paquete privado y ZIP
  final pasan el guardado en 32+64; #318 pendiente de integración. ZIP personal
  limpio debe entregarse siempre visible en la raíz
  `C:/Users/MP/Documents/00 Claude/KHCoMPC`, no solo bajo `build/releases`.
  Copia corregida allí, SHA-256
  `be1ed897f4e98c7be88ab6113692578b25b81b91d9f02823a64d7b49dd17fc81`.
  Aún sin prerelease pública. [Tablero actualizado](OPTIPORT_V21.md): 10/14
  originales (~71 % de trabajo de primera batería); migración/plataformas y UI
  añadidas se siguen aparte. No es un porcentaje de estabilidad.

Las notas fechadas anteriores que dicen «ninguna release nueva» o describen
otro orden de integración se conservan como historia y quedan sustituidas por
este mandato del 27/10.

**v2.0 estable publicada.** Confirmado en GitHub el 26/10/2026:
[release v2.0](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.0),
publicada a las 17:20 de Chile (`2026-10-26T20:20:29Z`, `isPrerelease=false`).
El usuario confirmó el lanzamiento y pidió limpiar la lista y preparar el tráiler.

## Retoma Codex — 27/10, robustez 2.1 y cierres (prioridad actual)

- [ ] **Prioridad: #277 Deck / #272 Android.** Investigar por separado: foto #277 = puerta de evento de Ciudad de Paso con spotlight; NO puerta del castillo. Log #272 = evento 74 Monstruo, ROM USA, Galaxy Tab S6 Lite / Android 13. Las pruebas anteriores hasta selección de mundo/tutorial no cubren la puerta fotografiada. Tres comentarios en cada issue al volver a consultar; sin log nuevo.
- [ ] **Cierre del usuario en Monstruo**, primer/segundo combate al volver a la ventana. Quiver v2.0 EU, Undub, HUD HD. Instalación principal `C:/Users/MP/Documents/Apps/QuiverLauncher/Apps/KingdomHeartsChainOfMemories-ReKHCoM`: **SOLO LECTURA, nunca ejecutar, modificar ni actualizar allí**. Copia del log/config/save en `khcom-hd-reload/build/codex-evidence/quiver-crash-20261027`. Log previo ya reemplazado al reabrir, sin dump/evento de crash reciente encontrado; RAM no confirmada como causa.
- [ ] **Repaso completo 2.1**, autorizado con decisiones técnicas delegadas: fallbacks, profiling, gestión de recursos, asíncrono/precarga, streaming y frame pacing. Algo más de peso en assets HD nuevos. Primero depurar defectos; luego alternativas recuperables, sin ralentizar lógica GBA. Alcance y orden en [PLAN_FALLBACKS_V21.md](PLAN_FALLBACKS_V21.md). No declarar ausencia global de bugs.
- [x] **Logs actual + cinco anteriores**, rotación y sesión fechada: integrado PR #282, SHA `5a523c61e`. Harness productor real, errores de rotación, shell y builds Windows/guest pasan. Guest freestanding no usa reloj libc; host conserva fecha.
- [ ] **Bucle de música HD sin progreso**: reproducción de bloqueo y corrección local RED→GREEN (decodificador vacío, dispositivo rechaza datos, reproducción normal, loop corto, loop vacío). Vuelve a música GBA sin silenciarla. Faltan integración/PR y regresión de audio completa; no atribuirlo aún a #277/#272.
- [ ] **Rendimiento HD**: nuevo arnés serial `profile_hd_resources.py` guarda frame times, cold/warm, RSS/private y hashes. Primera corrida quedó en selector de modo oculto (config de fixture=Choose); corregido a modo explícito, no tomarla por medición válida. Retomar tras atender cierres. GPU oculta y audio dummy; memoria de proceso no equivale a VRAM.
- [ ] **Protección del progreso ante fallos**: auditar guardado atómico y recuperación separada de slots manuales; no intentar continuar memoria corrupta ni afirmar que existe recuperación automática aún.

Rama propia `fix/crash-diagnostics`, checkout `khcom-crash-diagnostics`; toma remota `crash-diagnostics-20261027` / `codex-crash`. Se conserva `fix/v21-bugs` de Claude y su lista detallada. Workers autorizados por usuario, esfuerzo bajo, tareas acotadas; builds seriales/silenciosos y ninguna release nueva.

**Actualización OptiPort:** [tablero de 14 bloques](OPTIPORT_V21.md), skill [pública](https://github.com/MatyMeatBoy/OptiPort). Integrados #283–#289: layouts/bounds, sprites, paletas, alineación, readiness HUD, feed musical y vídeo Android/MF. #277 pasa entrada completa Windows/Linux64; no declarar confirmación del reportero ni #272 resuelto. Usuario autorizó integrar y enviar builds de prueba a ambos reporteros, luego continuar la batería. Host musical, vídeo Linux y preservación SRAM ante error de flush están en carriles separados.
## Bugs a resolver antes de v2.1 estable — 27/10 (Claude)

- [x] **#277 Deck: cierre al abrir la puerta** — arreglado por Codex en PR #286 (`DOOR_PORTABILITY_277.md`):
  alineación real de símbolos ROM en cabeceras (`gen_pc_assets.py --ensure`) + paletas de cartas agotadas. Claude
  llegó a la misma causa (`movaps` en `LoadMapSelectKindPalette`) y retiró su versión duplicada (ROM_ALIGN).
  `test277b` (sin arreglo de paletas) reemplazado por **`test277c`** (main con #286, `build_linux.py --package`),
  SHA `c6125f70…309c`; VM 64 bits con la ruta de Codex (`doorpose:3`) llega a `world=10`. Avisado en #277.
  Falta: confirmación del reportero; #277 quedó cerrado por el merge de #282 (reabrir requiere OK del usuario).
- [ ] **#272 Android**: probablemente otra causa (AArch64 tolera cargas desalineadas; armv7 sin NEON ahí).
  Esperar datos del reportero; build de prueba igual.
- [ ] (histórico) **#277 Deck / #272 Android: cierre tras la Llave del Inicio** (puerta del castillo → selección de mundo).
  Windows EU pasa el punto (llega al combate tutorial con A repetida). Sospecha: camino portable (host 64 bits /
  Android). Reproducir en VM Linux 64 y arreglar; luego barrer el juego entero en 64 bits.
- [ ] **#42 desync del final HD de Sora**: rama `fix/ending-cutscene-desync` (Codex), sin integrar.
- [ ] **#163 galería**: vídeo del reportero (youtube qkZSuuTsrio) sin revisar todavía.
- [x] Tests lentos con `KHCOM_FAST` + vsync=1 (el present esperaba al refresco): FAST fuerza vsync 0.
- [ ] Batería: arneses que piden builds viejos (`build/rel-eu`, `codex-gameplay-*`…) y fallos reales a separar:
  `test_hud_crop` (3840x1080 mapa), `test_ppu` 7/12, `test_door_views` (falso positivo con «VRR-path=unavailable»).
- [ ] **HUD HD activado en pleno combate**: tirones como de carga al volver de Opciones. Precargar el HUD HD
  antes de devolver el control al juego.
- [x] **Carta de recarga HD**: número bajo el relleno = v2.0; arreglado en main por #276 (Codex). Verificado 27/10
  con `test_hd_reload_order.py --hd-hud 0|1` (contador visible, 1383 px blancos, también con HUD HD).
- [ ] **HUD HD: «EXP ganada»** no se reubica y queda tapada por la vida nueva.
- [ ] Linux 64 VM: partida nueva → puerta del castillo → selección de mundo → combate tutorial **sin fallo**
  (sin música HD). Repetir con música HD (`.ogg`), que el reportero sí tiene.

- [x] **Tienda Moguri (Cambiar naipes), números HD** (27/10): el «2» salía sobre un bloque gris. Causa:
  `LoadDecimalDigitTiles` (mode_ms_top.c) dejaba que port_ppu dedujera fondo/tinta/sombra de cada tile por
  frecuencia; el 2 tiene más tinta que fondo → colores invertidos. Arreglo: todos los dígitos usan los colores del 0
  de su juego (gris o blanco). `test_moogle_charge_digits.py` (RED 71/71 → GREEN 40/39; ceros grises intactos).
  Mismo render: «Naipes activos 021/062» y total Moguri correctos.
- [ ] **Cartas HD pequeñas (lista de naipes): encaje de la imagen** como se ajustaron las de llave. Marcadas por el
  usuario: Simba (retrato desborda/cortado), Genio (asoma por la derecha), carta verde de objeto (cubo azul
  descentrado), carta gris de enemigo (sombra/Soldado). Revisar todas las cartas, no solo esas.
- [x] Tienda Moguri en vista de tienda (fila de 5 cartas con número): números HD se ven bien (captura usuario 27/10).
- [ ] **HUD HD (solo modo HUD HD; el HUD normal no se toca):** — tareas en la cola de arte (`hudhd-*`,
  `cartas-hd-encaje`, `escenas-retrato-x4`), registradas 27/10 para revisión en Atlas.
  Captura base 27/10: `build/hudshot` en khcom-hd-reload, ini del usuario (copia de `build/codex-evidence/
  quiver-crash-20261027`) con `hd_hud=1`, `fullscreen=0`, `KHCOM_WINDOW=1536x864`, `KHCOM_BATTLE=204:8:0`,
  `KHCOM_MONKEY=19:120`. Los números bajo el retrato y la pila del jefe solo salen al apilar/elegir: usar
  secuencia de combate con stock (`KHCOM_KEYS` con L/R de apilar). Son cambios visuales grandes: proponer en Atlas
  (regla «visuales a la app») antes de integrar; solo modo HUD HD.
  - Pila de cartas: reubicarla según la vida nueva y dibujarla **encima** de ella; también se les pierde el **borde**.
  - Igual para enemigos/jefes si aplica.
  - Números de las cartas: **dorado y azul no se ven abajo**; **dorado no se ve arriba** (la azul arriba sí).
    Parece HUD mal posicionado o vista incompleta. Arreglar limpio.
  - «EXP ganada» tapada por la vida nueva — ARREGLO en PR (27/10): el anillo HD de vida (cartas HD + gBtlHpRing)
    tapaba «SIG. NIVEL»/«EXP +n»; btl_exp.c baja la línea 7 px GBA solo con el anillo HD (`PortHdHudExpDrop`).
    Sin anillo, igual que el cartucho. Captura del usuario 27/10 (combate en Monstruo).
  - Pila de cartas apiladas junto al retrato (p. ej. «7 4 6», total 17 encima): en HUD HD salen SIN marcos HD y con
    el número dorado (premium) en GBA; deben llevar marcos/dígitos HD como las cartas en mano (captura 27/10).
    Pista: las cartas pequeñas HD son grupo 6 (`SmallCardHD`), que conservan el marco del cartucho
    (`PortHdSpriteKeepsCartridge`, port_ppu.c:2366/4915); en la pila del HUD el marco no se ve. Capturar con
    secuencia de apilar (L+R / stock) en HUD HD y revisar si `bandOnly`/capa HUD descarta el marco.
- [ ] **Set HD de rotura de cartas** (cortanaipes, bloqueo/contranaipe y afines): el usuario recuerda haberlo
  visto listo; buscarlo en hd_sprites.tsv/ramas/Atlas y comprobar que se dibuja.
- [x] **Cortanaipes / Bloqueo / Recuperar HD no se ven (CAUSA 27/10, ARREGLADO con el punto del launcher)**: el commit 428aac55b (26/10, pad= en avisos
  de combate) cambió la proporción de sus recetas; las imágenes importadas antes quedan con la proporción vieja y
  `Load` (port_hdsprites.c) las rechaza a propósito («rejected … recipe»: no estirar). Comprobado con el import de
  `D:/01Claude/ReKHCoM-build/hd-completo/sprites`: `gBtlPop{CardBreak,Guard,Recover}SpanishTiles` rechazados → se
  ve el aviso GBA. Solución inmediata del usuario: reimportar HD en el launcher. Solución de fondo = punto siguiente.
- [ ] `HD sprites: gWorldselectTitleSpanishBanner is 960x128, its recipe wants 8.688`: aviso repetido en selección de
  mundo (VM 27/10). Revisar receta o el importador.
- [ ] VM Linux 64 con **todo HD activado** (texto, HUD, música, escenas, cartas, título, subtítulos): partida nueva
  hasta combate tutorial **sin fallo**. Falta: datos de los reporteros (ajustes exactos, vista, log, modo).
- [ ] **Importación de assets a prueba de updates**: importar siempre todo lo que podamos necesitar desde la
  instalación de Steam (Re:CoM/HD); si una versión nueva pide un asset que falta, que el launcher lo detecte y
  ofrezca re-importar solo lo faltante desde Steam.
- [x] **#42** desync final HD: ya integrado en main (PR #273); sale en 2.1.
- [x] **#163 video galería** (qkZSuuTsrio, grabado con v2.0): pausa de película HD en la galería **sin texto** =
  bug #274, arreglado después de v2.0 (PR #274, 22:50Z). Mismo `MoviePause` en ambos bucles de `PlayFile`.
  Test `port/tools/test_hd_gallery_pause_text.py` (27/10): pausa de la película HD del opening (513) desde la
  pestaña ReKHCoM de la galería, en título HD/GBA y en partida: texto HD legible. PASS.
- [ ] **Atlas: escenas de diálogo con retrato en x4 1080p** (ej. Hades «¡Regla n.º 6!» en Coliseo: el retrato
  sobresale del cuadro y la caja no ocupa la vista amplia) nunca entraron al Atlas. Buscar las similares y enviarlas.
- [ ] **HUD HD vs jefes**: el nombre del hechizo que castea el jefe (p. ej. «15A Locura» junto a su carta) sale
  cortado/mal ubicado; ajustar posición con su pila y subir la pila del jefe para calzar con el HUD HD (arriba
  derecha) y mejorarla en HD. Solo en modo HUD HD.
- [x] **Sonido de recarga del mazo enemigo con undub** (27/10): NO se reproduce. Combate Jafar 9000 cuadros, mismas
  entradas, `KHCOM_SONGLOG=1` + `KHCOM_WAVDUMP` con `undub=0/1`: los mismos 491 sonidos en el mismo cuadro; solo las voces
  (`SONG_VO_*`) cambian; `SONG_SYS_CHAGE` (carga/recarga, 112 veces) igual RMS (0,0587 / 0,0590); nada queda mudo.
  Si el usuario vuelve a notarlo: pedir el combate/jefe exacto para repetir la medición allí.
- [x] HD de «nivel plus», match de contranaipe y bloqueo: el usuario confirma que se ven bien.
- [x] **Launcher 2.1: comprobación rápida de assets del mod** — sprites HD hecho 27/10: `recipes.txt` con huella por
  fila al importar; al arrancar `HdStaleSprites` cuenta lo desfasado (huella distinta, o en imports viejos proporción
  del PNG leyendo solo su cabecera) y reimporta solo gráficos desde la instalación. `--hd-stale` para pruebas;
  `test_launcher_stale_sprites.py` PASS. Import real de hd-completo: 36 desfasados → reimport → 0; Cortanaipe HD
  vuelve (0 rechazos). Falta: lo mismo para vídeos/música si algún día cambian.
  Plan concreto (27/10): al importar, escribir `hd/sprites/recipes.txt` con «nombre<TAB>hash de su fila de
  kHdSpriteRecipe»; al arrancar el launcher, comparar con las filas actuales (barato: sin abrir PNG) y, si alguna
  cambió o falta, ofrecer/hacer reimportación solo de esas filas desde la instalación de Re:CoM recordada. El juego
  ya rechaza la imagen vieja (no se estira), así que el usuario hoy ve el GBA sin aviso: añadir aviso en log/launcher. al instalar/verificar: la primera vez revisa todo;
  después solo detecta cambios o faltantes (marcas de tamaño/fecha/hash guardadas), sin demorar cada arranque.
  Si falta algo, ofrecer importar lo faltante (ver punto de importación desde Steam).
- Prueba 27/10 (hipótesis import incompleto): VM 64 bits con `zz_event_001.mp4` de 0 bytes y `zz_event_011.mp4`
  truncado (100 KB) en `hd/movies` y `hd/scenes`: **sin fallo** hasta el combate. Siguiente: import antiguo
  completo (otra estructura de carpetas), sprites HD viejos, música ausente a mitad, ROM US/JP en 64 bits.
- Issues 27/10 03:00+: sin respuestas nuevas de los reporteros todavía (#277, #272).

## TRASPASO (Claude, 27/10): cómo retomar

**Prioridad 1: el cierre de #277 (Deck) / #272 (Android) tras la Llave del Inicio.** Crítico.
**REPRODUCIDO 27/10** en VM Linux 64 (guest EU): `KHCOM_MODE=60:room:10901,120:fieldfreeze,200:doorpose:0`,
`KHCOM_KEYS=240-242:A` + A cada 40 frames desde 420 → `Segmentation fault` del host al abrir la puerta
(Windows con las mismas teclas entra a la sala y al combate en frame 1424). El recorrido completo antes paraba
en el combate tutorial y por eso no llegaba. #277 lo cerró por error el merge del PR #282 (logs): reabrirlo
necesita OK del usuario.
- Hecho: Windows EU (texto HD) y VM Linux 64 (build de prueba #277, `D:/01Claude/ReKHCoM-build/linux/vmshare277`)
  con todo HD activado (música, escenas: solo ROP/zz_event_001/011 copiados, sprites, subs), en **español e
  inglés** EU: partida nueva → puerta castillo → selección de mundo → eventos 1,2 → combate tutorial, sin fallo.
- Hipótesis del usuario (probar primero): **import HD antiguo/incompleto**: el juego busca un vídeo/escena o
  asset que no está y no tiene ruta de respaldo → crash. Probar en VM: share con `hd/` de un import viejo
  (p. ej. carpetas en `D:/01Claude/ReKHCoM-build/_para_borrar_20261026/` o jugar/pre17), `hd/scenes` con mp4
  ausentes/corruptos/0 bytes (zz_event_002/012…), `hd_scenes=1` con escenas faltantes. Si es eso, arreglar en
  todos los caminos (vídeo, música, sprites): ausente o ilegible → seguir con lo GBA, nunca cerrar.
- Rastrear si esos reporteros ya jugaron bien antes (comentarios previos en #277, #140, #49, #77 de Gameboy121;
  #272 de uxshade) y qué cambió entre esa versión y la que falla.
- Ya pedidos en los issues: ajustes, modo Juego/Escritorio, logs, packs HD, región, y si con HD apagado falla.
- Cuando se reproduzca: arreglar, test de regresión, build de prueba a ambos (rama orphan `test-builds`, nunca
  release) con gracias y disculpas; en Deck mencionar que la build Windows va bien con Proton.

Rama `fix/v21-bugs` (sin PR). Lista de arriba = alcance de v2.1 estable; para cada bug, causa y por qué el arreglo
cubre todos los casos. VM: `MSYS_NO_PATHCONV=1 PYTHONUTF8=1 python port/tools/linux_vm_test.py --share
D:/01Claude/ReKHCoM-build/linux/vmshare277 --fresh-share --disk-size 4G --run-timeout 1400 --run "cd
/root/share/ReKHCoM && rm -f kh_com.sav && SDL_VIDEO_DRIVER=offscreen SDL_AUDIO_DRIVER=dummy KHCOM_FAST=1
KHCOM_TRACE_MODES=1 KHCOM_KEYS=$(cat keys.txt) KHCOM_QUIT=16000 ./ReKHCoM.sh"`. `keys.txt` en el share = A cada 50
frames (ahora ruta inglés: A en 200). El paquete solo trae guest EU. Builds locales: `build/pc` (EU),
`build/pc-us`, `build/pc-jp` en khcom-hd-reload; ninja en `../toolchain/bin/ninja.exe`.
Tests: siempre `SDL_AUDIO_DRIVER=dummy`; el 27/10 algo sonó en la batería → localizar.

- [ ] Tests nunca deben sonar: algo sonó el 27/10 en la batería; localizar qué test no usa audio dummy.

## Relevo — Claude, 26/10 noche (Codex durmiendo)

- **Limpieza:** worktrees viejos respaldados en GitHub (`backup/*-20261026`) y quitados; quedan `khcom` (base),
  `khcom-claude-int`, `khcom-review-app` (Atlas), `khcom-codex` (solo datos de Atlas en `build/art-review`),
  `khcom-hd-reload` y `khcom-trailer-production`. Partidas de copias de prueba en `KHCoMPC/respaldos/saves/`. Lo de
  D: que ya no sirve, junto en `D:/01Claude/ReKHCoM-build/_para_borrar_20261026/` (borrable entera).
- **#276 → #278:** máscara del dorso HD de la recarga (el relleno ya no invade borde ni corona).
- **#277 → #279:** Steam Deck sin texto HD: el host de 64 bits no indicaba `KHCOM_ASSETS`. Probado en VM (Vulkan).
- **#272 (Android, Monstruo):** PC pasa el evento 74 y entra al mapa; pedido modelo/Android/log al reportero.
- Verificado de nuevo: pausa de cinemáticas HD con texto (#274) en 5 idiomas; regresiones de recarga y cola JP.

## Traspaso — Codex, 26/10: desync del final HD (#42)

Corrección del preroll PCM de Media Foundation en rama `fix/ending-cutscene-desync`,
tarea remota `ending-desync-42` (owner `codex-ending`). Causa, reproducción nativa,
vecino 123/170 y evidencia RED/GREEN: [ENDING_SYNC_42.md](ENDING_SYNC_42.md).
No hay nueva release ni cambios al archivo importado.


El cierre y los traspasos previos se conservan en
[archivo/PENDIENTES_CIERRE_V20_20261026.md](archivo/PENDIENTES_CIERRE_V20_20261026.md).
No volver a preparar ni publicar la 2.0 por esas notas. Tampoco convertir sus pruebas
sin evidencia en verificadas: se archivan como registro histórico.

## Prioridad: tráiler v2.0

Directrices completas: [TRAILER_V20.md](TRAILER_V20.md).

- [ ] Seleccionar material de la estable y versiones exactas **aprobadas con honores** en Atlas.
- [ ] Grabar comparativas a x4: sprites y fondos GBA en la vista ampliada → slider → versión moderna.
- [ ] Destacar title screen HD, arenas ampliadas, HUD HD y música HD; dedicar un tramo a Jafar.
- [ ] Recuperar efectos, transiciones y continuidad del tráiler anterior; montar, revisar y exportar.
- [ ] Preparar miniatura y descripción actualizadas; publicación del vídeo pendiente de indicación del usuario.

Estado: directrices registradas; todavía sin nuevas capturas ni montaje v2.0.
Carril documental: `docs-v20-trailer`, `codex-trailer`, rama `codex/v20-trailer-pendientes`.

## Asuntos posteriores que conservan seguimiento

- **Indicaciones de mando experimentales 2.1:** primera integración textual,
  opt-in y fiel al binding efectivo; segundo incremento incorpora símbolos
  pequeños, tokens JP/compuestos conocidos y preset confirmado Nintendo A / PS
  Circle. Refresco de símbolos semánticos abiertos en
  [CONTROLLER_PROMPT_LIVE_SYMBOLS.md](CONTROLLER_PROMPT_LIVE_SYMBOLS.md);
  quedan Original→activar/fallback textual, metadata real de mando en
  guests, cobertura de frases y revisión GPU. Detalle y pruebas en
  [CONTROLLER_PROMPTS_EXPERIMENTAL.md](CONTROLLER_PROMPTS_EXPERIMENTAL.md).
  No dar por completo el alcance del usuario con la entrega textual.

- **HUD HD, reporte 27/10:** estabilizar borde/contador del medidor enemigo;
  conservar aparte la equivalencia de capacidad HP del jugador (evidencia
  155/155 frente a anillo fallback normalizado), longitud por máximo del
  enemigo y bordes internos dentados al caer fuera de supersampling.
  Evidencia, pruebas y siguientes capturas en
  [HUD_ENEMY_STATIC_TRACK.md](HUD_ENEMY_STATIC_TRACK.md). Paridad exacta con
  Re:CoM 1.5 y revisión visual GPU todavía pendientes; no darlo por cerrado
  únicamente por la prueba CPU ni trasladar un porcentaje optimista.
  Recorte nativo medido y límite pendiente de Windows 64/guest detallados en
  [HD_GAUGE_VISIBLE_BOUNDS.md](HD_GAUGE_VISIBLE_BOUNDS.md): el guest no compila
  la cadena SS actual; requiere trabajo de host/bridge separado. No presentar
  el menor rectángulo como arreglo ya confirmado de su captura dentada.

No son bloqueos del lanzamiento ya realizado. Confirmar su estado actual antes de actuar;
no iniciar nuevas baterías ni seguimientos automáticos por esta limpieza.

- Arte: atender comentarios nuevos de Atlas cuando se retome; «aprobado con mejoras» sigue cerrado.
  Las reservas remotas antiguas no prueban que una pieza siga abierta. Consultar Atlas y GitHub;
  no alterar valoraciones, comentarios ni reservas ajenas para limpiar esta lista.
- Respuestas externas del lanzamiento (#140, #44, #163, #202) y anuncios: comprobar cuáles ya
  se enviaron antes de repetirlos. Esta tarea documental no envía mensajes.
- Esperando material/respuestas: revisión de galería y vídeos de #163, prueba CRT de #202,
  material PS2 con REA; reportes de pelo de Sora, cuadro negro de Agrabah y Vita/Undub,
  únicamente si siguen reproducibles en la estable. No darlos por corregidos sin evidencia.
- Futuro: mejorar modos PS2 existentes con REA; GPU/potencia de consolas con ajustes documentados;
  iOS con Mac; Switch; online; rutas de Sora y Reverse/Rebirth (#39). Sin porcentajes heredados.

## Reglas vigentes

- Rama y PR por entrega; tomas remotas mediante `art_queue.py`. No push directo a main.
- Arte: versión exacta y valoración humana en Atlas; una hija no hereda honores.
- Conservar partidas, comentarios, imágenes, WIP, releases y archivos del usuario.
- Probar oculto y sin sonido; responder en español y breve.
