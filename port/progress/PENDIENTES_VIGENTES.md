# Pendientes vigentes — preparación de 2.2

**Mandato actual, 27/10/2026:** el usuario traslada lo restante a **2.2**.
[Plan y traspaso](PLAN_V22.md): OptiPort conserva una base provisional de
**10/14 bloques (~71%)**, con cuatro parciales; no es porcentaje de estabilidad.
2.1 ya está publicada. #356 está sólo en main y se entregará en una build
posterior, no en el ZIP 2.1 existente. El resto de este documento conserva
los detalles/historia de cada pendiente y no autoriza reabrir la batería de 2.1.

## P0 — #360: 64 bits cierra el launcher (Windows 11 x64, Pedro)

- [x] **Caída a 32 bits + registro detallado (PR #368):** cualquier crash de 64
  bits abre un aviso del lanzador (jugar en 32 / reportar / cerrar); Automático
  recuerda el crash por versión. `kh_com.log` lleva equipo, excepción, módulo u
  offset del guest, registros y pila; el reporte junta la sesión y las 5
  anteriores en `ReKHCoM-reporte.txt`. Build de prueba enviada a Pedro:
  prerelease `v2.1-test360` (instalación limpia). Esperando su resultado/log.
- [ ] Falla en Automático y en Forzar 64; Forzar 32 funciona. Pedidos datos
  (CPU/GPU/RAM, ruta, región, HD, logs, Visor de eventos) en el issue, 27/10.
  **No reproduce aquí** con el ZIP 2.1 (lanzado como el launcher: guest,
  `--data`, `--boot-status`, `KHCOM_ROM`): instalación limpia EU/US/JP,
  actualización 2.0→2.1 en modo HD con import completo, y ruta con acentos
  (`Transferências`). Pista: Automático sólo vuelve a 32 bits con códigos de
  arranque (2/3/4, 0xc0000135/7b/142, `runtime_selection.h`); un crash de 64
  bits (p. ej. 0xc0000005) antes o después del marcador no reintenta, y eso
  encaja con el reporte. Esperar logs; posible mejora: en Automático, volver a
  32 bits si 64 bits muere por excepción en los primeros segundos.
  Carpetas de prueba: `pruebas/issue360*` (la `-upg` tiene junction `hd`:
  deshacerla antes de borrar).

## TRASPASO (Claude, 27/10 tarde)

Prioridad 1 cerrada salvo guante y botón A (abajo). #360: esperando el log de Pedro con la build
`v2.1-test360`. Arnés de capturas usado: `kh-worktrees/hd-small-card-top/build/cards-qa/run_cases.py` (64 bits,
HUD HD con `KHCOM_HD_HUD_DEV=1`, `KHCOM_HOST_SHOT`) y `blink.py`/`premire.py` (32 bits offscreen).
El usuario pide seguir hasta vaciar pendientes sin preguntar.

## PRIORIDAD 1 — Cartas (usuario 27/10, pasan directo por Claude, NO por Atlas)

Sacadas de los preliminares de Atlas (9 piezas; respaldo de la base en
`respaldos/revisiones-antes-cartas-20261027.sqlite3`, evento `moved_to_pendientes`).
Claude captura en juego (offscreen), corrige, verifica y abre PR; muestra
antes/después al usuario en el chat. Comentarios del usuario entre comillas.

- [ ] **Cartas pequeñas HD: encaje (lista de naipes, 105).** r1: 8 nubes «mal
  encuadrado» + «hay muchos sin centrar, hazlo pro». r2 (recorte de la carta
  grande) y #359 (ventana completa centrada) ya integrados. 27/10: re-import
  con la receta de main y hoja de las 105 revisada: todas con el encuadre de su
  carta grande, centradas; lista de naipes en juego correcta. Falta que el
  usuario lo confirme en su instalación (re-importar HD).
- [x] **HUD HD: largo de la barra del enemigo** — PR #365: barra entera según
  vida máxima como Re:CoM (`FUN_14020d270`: cuerpo = máximo de la barra superior,
  superposición sólo en la superior de varias). 560 HP por barra = escala de
  Sora; ambos crecen con su máximo (pedido del usuario). `test_hd_enemy_gauge.py` PASS.
- [x] **HUD HD: nombre del hechizo del jefe** («Muro de fuego» de Axel sobre
  sus cartas): con cartas HD y lienzo amplio, el nombre termina antes de la pila
  (ancho real del sprite; pila en x del total + margen completo del lienzo).
  GBA intacto. PR `fix/hd-boss-sleight-name`, capturas Axel 1280x720 antes/después.
- [x] **HUD de combate HD: borde verde interior granulado** — era la captura
  histórica v2.0. Con el antialias de texturas GPU (antes de 2.1) el anillo y
  la barra de Sora salen limpios a 1280x720 (64 bits) y 1920x1080 (32 bits 2.1).
- [x] **Síntesis de estancias: carta de requisito HD** — arte aprobado por el
  usuario 27/10. El import compone las 405 piezas desde `FO0102 UK_room01.imz`
  (carta con corona teñida como cristal, bisel blanco, dígitos cursiva con
  sombra, flecha/igual a juego); nueva operación de receta `fade` en ambos
  importadores; nada de Re:CoM en el repo. Verificado en juego (W64 EU). Los
  usuarios ya importados las reciben por la reimportación de sprites desfasados.
- [x] **Añadir naipes: cantidad HD y selector** — ya resuelto en main por la
  máscara de cobertura por cuadro (#124): medido 27/10 cuadro a cuadro 480..540,
  la tinta GBA y la HD suben y bajan en los mismos cuadros; el borde del
  selector recorta las mismas filas.
- [x] **Bonus élite: cabecera** — la cabecera HD sale completa (captura 27/10).
- [ ] **Guante HD (cursor de mano) en todos los menús** — Re:CoM lo trae en
  `room01`/`pause` (apunta horizontal a la derecha); el GBA `gSmallHandCursor`
  apunta en diagonal abajo-izquierda con 8 cuadros (0-3 sube/baja, 4-7 se
  desplaza). Falta: operación de rotación libre + espejo en ambos importadores,
  recetas por cuadro (`frame`) y captura en Bonus élite, barajas y diálogos.
- [ ] **Botón «A» de los avisos en HD** — usar el sistema de prompts de mando
  (Original/Nintendo/Xbox/PS) para el icono de Bonus élite; comprobar en captura.

- [ ] **Noticias en la ficha de Steam para ReKHCoM** (usuario 27/10): la zona de actividad queda vacía porque es un acceso directo no Steam. Sin vía oficial; alternativas a investigar en [PLAN_V22](PLAN_V22.md) (P3).

## Estado publicado y limpieza — 27/10, después de la entrega

Repaso posterior acotado: [OptiPort actualizado](OPTIPORT_V21.md) y doce
mediciones del paquete final registradas; sin crash nuevo. La variación de
pacing no justifica un cambio especulativo. Próximos checks concretos aún sin
cierre: nombre de ataque del jefe frente a barra/pila; retorno de galería sin
flash título/partida×GBA/HD×normal/skip; dígitos
premium/modificados presentes/ausentes. No repetir primero la matriz de
submenús ya capturada. El usuario pidió cortar la ampliación de pruebas al
comunicar la release; los paquetes publicados permanecen intactos.

- [x] **Fondo mínimo sinposter:** variantes GBAazul/HDdorado implementadas y
  revisadas en juego real; ceroIO/asignaciones. Poster real conserva prioridad.
  [Evidencia y capturas](GALLERY_EMPTY_BACKDROP.md). Ajuste posterior a la
  publicación2.1, sin reemplazar archivos de la release. No cierra los checks
  separados de latencia de previews ni retorno sinflash.

**[2.1 estable publicada](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.1)**
el27/10 a13:02:10UTC, tag `8c613ae5d`. PR351/353 integrados; Windows, Linux/Deck,
Android beta y Anbernic beta entregados con manifiesto/hashes. Los motores
corresponden a `cd144335e`; launcher/empaquetado Linux a `de434db7c`.
El helper nativo Linux pasa7/7 transacciones y empaquetado en Debian12VM;
75 archivos nativos permanecen idénticos tras reempaquetar. Android final
compilado; no se repitió el gameplay del conjunto final Linux/Android tras
la baseline35b57818c. No confundirlo con confirmación física de reporteros.
Respondidos #42/#163/#277/#272/#330/#336 con la release. #330 y #336 cerrados:
setupGamingMode y vía nativa de importación entregados; no se afirma haber
diagnosticado el crash Wine. #272 sigue abierto esperando resultado Samsung.
Las notas anteriores de «sin release nueva» quedan como historia.

Primera limpieza de dummies/temporales: retiradas4 copias de ejecutable,
biblioteca o guest de runtimes de QA tras comprobar SHA256 idéntico a una
copia conservada del paquete; liberados15.645.146bytes. Dos binarios distintos
se conservaron como evidencia histórica. Registro local:
`khcom-crash-diagnostics/build/v21-dummy-cleanup.json`.

- [ ] Continuar limpieza por tarea terminada: comprobar referencias/uso antes
  de retirar placeholders o fixtures. Los backends SDL `dummy`, espaciadores
  ImGui, debug original y stubs de ABI no son basura por su nombre.
  Conservar logs/capturas de regresión, ROMs, partidas, WIP y archivos ignorados
  necesarios antes de archivar cualquier worktree.

## Cierre de entrega 2.1 — mandato más reciente, 27/10

El usuario pidió integrar todos los PR listos y publicar **2.1 estable** ahora,
y acotar la verificación para no seguir retrasando la entrega. Esto sustituye
el orden anterior de prerelease y la exigencia de completar cada ampliación
antes de publicar. No equivale a cerrar sin evidencia los pendientes restantes.

Conjunto de código `cd144335e`, PR #351: incluye los heads exactos de todos
los PR abiertos #331/#332/#334/#335/#338/#339/#340/#341/#342/#345/#346/#347/
#348/#349/#350 y #352, más correcciones de integración. Windows final EU/US/JP
32/64 compilado; ZIP extraído pasa guardado privado, ocho salas Sora/Riku y
puerta de Traverse en ambas arquitecturas. Monstruo World5 evento74 USA pasa
en ambas. Nueve casos MF de vídeo pasan. Capturas GPU reales verifican stocks
1/2/3 de jugador/enemigo y totales visibles con la corrección final. Linux
desktop32/64 y Anbernic armhf compilados; Android se cierra por separado.
No se amplía ahora la batería ni se promete completar campaña o cero crashes.

Conservar abiertos como seguimiento: confirmación en Deck/Samsung originales,
crash de foco Monstruo v2.0 aún sin reproducción, mediciones comparables de
p95/p99 y memoria, QA física/hotplug de controles experimentales, detalles
visuales sin evidencia de cierre y limpieza posterior de worktrees. Las notas
históricas siguientes no sustituyen este estado; no borrar WIP ni fixtures.

## Mandato vigente de coordinación — 27/10/2026

- [ ] **Launcher: audio/vídeos HD EN/JP y disponibilidad clara**, tarea remota
  `launcher-hd-media-language-selection-v21`. Primer incremento de detección ACF,
  señales de fuente/huella y guía/reescaneo integrado #327. Selección/imports/playback
  por idioma en #335/#342, todavía en revisión; [fuentes y traspaso](HD_MEDIA_LANGUAGE_SELECTION_V21.md). Steam
  permite elegir idioma del juego y descargar archivos mediante su cliente;
  no hay automatización de depots/cambio de idioma verificada para ofrecer.
  Separar idioma de texto/subtítulos, Undub GBA y audio HD; el import actual no
  guarda idioma de origen y puede conservar una variante previa al reimportar.
  Guía con enlaces oficiales + reescaneo, sin cambiar Steam ni descargar ahora.

Consultar [COORDINACION_OPTIPORT_V21.md](COORDINACION_OPTIPORT_V21.md) antes de
tomar trabajo: **Codex termina OptiPort al 100 %, prerelease Windows 64 y ZIP
personal limpio; Claude puede tomar todos los pendientes regulares** (HUD,
jefes, recetas, Atlas, galería, etc.), reservando en `coordination/art-queue`,
preservando WIP y abriendo PR. **Última actualización del usuario:** Codex recoge
e integra los PR regulares ya listos antes del ZIP Windows, con revisión y lock;
entrega el ZIP prioritario y continúa hasta 2.1, incluyendo los pendientes
regulares y los PR verificados. No esperar a cerrar el método entero para
resolver un bug. Los claims activos de core/guardas, arquitectura y QA se respetan;
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
  el UI actual. Identificados GBA, ReKHCoM y variante híbrida: #342 en revisión,
  apilado sobre #335. Continúa pendiente medir en los modos solicitados:
  OptiPort mide entrada, p95, memoria, caché, precarga/cancelación y calidad;
  Claude toma el fallback visual y presentación como tarea regular bajo claim.
  Objetivo: carga casi instantánea, poca pérdida de calidad y evitar esperas
  largas al entrar a escenas. El PR implementa caché/fallback, no prueba aún
  latencia p95 real ni calidad visual final; ver
  [coordinación](COORDINACION_OPTIPORT_V21.md#pendiente-vista-y-previews-en-ambos-modos).

- [ ] **Cierre posterior a 2.1: ordenar worktrees** según el documento de
  coordinación, conservando WIP, partidas, ROMs, evidencia y commits. Ahora
  solo inventario/registro; no mover activos ni borrar. Nuevos worktrees:
  preferir `KHCoMPC/khcom-worktrees` según la raíz del repositorio.

- [ ] **Galería: retorno directo al terminar/saltar escena**, sin destello
  intermedio de título/gameplay. Claude regular UI, dependencia de pacing
  OptiPort si aplica. Probar título/partida × normal/skip × GBA/ReKHCoM,
  capturando cuadros, restaurando UI/renderer/input/audio sin avanzar historia.
- [x] **Caché getenv estable del guest:** RED por rollover de 128 claves,
  corrección acotada y pruebas CPU integradas #343, claim
  `guest-getenv-stable-values`. [Contrato](GUEST_GETENV_STABLE_VALUES.md).
  No demuestra causalidad del cierre temprano Android; reconstrucción y QA
  del artefacto del coordinador siguen pendientes.
- [ ] **Prompts live Original/fallback:** implementación acotada y prueba real
  EU/US/JP en `feature/controller-original-live-spans`, hija de #338. Metadata
  de fuente y backups reversibles; `?` honesto en inputs no representables.
  [Evidencia y límites](CONTROLLER_PROMPT_ORIGINAL_LIVE_SPANS.md). Pendiente QA
  visual/paletas, hotplug físico y cobertura tutorial; no se declara cierre total.
- [ ] **Metadatos reales del mando en 64 bits:** implementación y regresión CPU
  en `feature/controller-bridge-metadata`, claim `controller-bridge-metadata`.
  Consulta opcional versionada sin cambiar ABI, caché por evento, fallback
  genérico en hosts antiguos. QA física/hotplug y unión con prompts aún pendientes;
  [contrato y evidencia](CONTROLLER_BRIDGE_METADATA.md).
- [ ] **Prompts de mando completos, experimental 2.1:** Auto, Original/GBA,
  Nintendo, Xbox y PlayStation; bindings efectivos coherentes con tutorial,
  convención Nintendo A/PS círculo elegible, assets existentes, todos los
  botones GBA de Opciones, overrides, hotplug y localización. #332/#334/#338/#340
  en revisión; quedan QA y activación desde Original en diálogo ya abierto.
- [ ] **Gate real del ZIP Windows 64:** primera puerta con guardado del usuario
  confirmada RED por coordinador, WER `0x100f7bd6`,
  `MapWallFaceIsUnreserved`, fila superior NULL/lectura BIOS GBA. Coordinador
  corrige fuente compartida; exigir GREEN del mismo repro en build final antes
  de entregar ZIP. Las ampliaciones UI anteriores no retrasan esa entrega.

  Actualización: #318 integrado; paquete e71 pasa seis regresiones (guardado,
  ocho salas y puerta de evento, cada una en 32/64). ZIP personal
  limpio debe entregarse siempre visible en la raíz
  `C:/Users/MP/Documents/00 Claude/KHCoMPC`, no solo bajo `build/releases`.
  Copia corregida allí, SHA-256
  `3645fe9013125be6722e28beb39e991ab27d874725044aa739a9dee2bdfb82a4`.
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

- [ ] **Prioridad: #277 Deck / #272 Android.** Mantener separados: #277 es puerta de evento de Ciudad de Paso, #272 evento74 Monstruo USA/Samsung Android13. La puerta correcta y el guardado pasan Windows/Linux32/64 y Android64 de prueba; faltan entrega nueva y confirmación en los dispositivos originales. Última consulta:5 comentarios en277,3 en272, sin respuesta nueva. Ver artefactos/resultados vigentes en [OptiPort](OPTIPORT_V21.md).
- [ ] **Cierre del usuario en Monstruo**, primer/segundo combate al volver a la ventana. Quiver v2.0 EU, Undub, HUD HD. Instalación principal `C:/Users/MP/Documents/Apps/QuiverLauncher/Apps/KingdomHeartsChainOfMemories-ReKHCoM`: **SOLO LECTURA, nunca ejecutar, modificar ni actualizar allí**. Copia del log/config/save en `khcom-hd-reload/build/codex-evidence/quiver-crash-20261027`. Log previo ya reemplazado al reabrir, sin dump/evento de crash reciente encontrado; RAM no confirmada como causa.
- [ ] **Repaso completo 2.1**, autorizado con decisiones técnicas delegadas: fallbacks, profiling, gestión de recursos, asíncrono/precarga, streaming y frame pacing. Algo más de peso en assets HD nuevos. Primero depurar defectos; luego alternativas recuperables, sin ralentizar lógica GBA. Alcance y orden en [PLAN_FALLBACKS_V21.md](PLAN_FALLBACKS_V21.md). No declarar ausencia global de bugs.
- [x] **Logs actual + cinco anteriores**, rotación y sesión fechada: integrado PR #282, SHA `5a523c61e`. Harness productor real, errores de rotación, shell y builds Windows/guest pasan. Guest freestanding no usa reloj libc; host conserva fecha.
- [x] **Bucle de música HD sin progreso**: integrado #288/#291/#295 con RED→GREEN (vacío, output rechazado, loop corto/vacío). Vuelve a música GBA. Mantener QA de audio/soak y no atribuirlo a #277/#272 sin evidencia.
- [ ] **Rendimiento HD**: nuevo arnés serial `profile_hd_resources.py` guarda frame times, cold/warm, RSS/private y hashes. Primera corrida quedó en selector de modo oculto (config de fixture=Choose); corregido a modo explícito, no tomarla por medición válida. Retomar tras atender cierres. GPU oculta y audio dummy; memoria de proceso no equivale a VRAM.
- [x] **Protección del progreso ante fallos**: #290/#314 integrados, SRAM preservada ante flush fallido y quickstates atómicos/namespaces. Falta QA de interoperabilidad del artefacto final; no promete recuperar memoria corrupta.

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
- Histórico sustituido: **#277 Deck / #272 Android: cierre tras la Llave del Inicio** (puerta del castillo → selección de mundo).
  Windows EU pasa el punto (llega al combate tutorial con A repetida). Sospecha: camino portable (host 64 bits /
  Android). Reproducir en VM Linux 64 y arreglar; luego barrer el juego entero en 64 bits.
- [x] **#42 desync del final HD de Sora**: #273 integrado `a3b8d7839`; conservar regresión seek/reloj/audio final.
- [x] **#163 galería**: vídeo revisado; pausa sin texto correspondía al fallo de cinemáticas ya corregido. #274/#275/#305 integrados. El nuevo destello al regresar es otro pendiente (#339).
- [x] Tests lentos con `KHCOM_FAST` + vsync=1 (el present esperaba al refresco): FAST fuerza vsync 0.
- [ ] Batería: arneses que piden builds viejos (`build/rel-eu`, `codex-gameplay-*`…) y fallos reales a separar:
  `test_hud_crop` (3840x1080 mapa), `test_ppu` 7/12, `test_door_views` (falso positivo con «VRR-path=unavailable»).
- [ ] **HUD HD activado en pleno combate**: tirones como de carga al volver de Opciones. Precargar el HUD HD
  antes de devolver el control al juego.
- [x] **Carta de recarga HD**: número bajo el relleno = v2.0; arreglado en main por #276 (Codex). Verificado 27/10
  con `test_hd_reload_order.py --hd-hud 0|1` (contador visible, 1383 px blancos, también con HUD HD).
- [x] **HUD HD: «EXP ganada»**: #313 integrado `434d6b8e2`; comprobar captura final con anillo HD, no duplicar implementación.
- [ ] Linux 64 VM: partida nueva → puerta del castillo → selección de mundo → combate tutorial **sin fallo**
  (sin música HD). Repetir con música HD (`.ogg`), que el reportero sí tiene.

- [x] **Tienda Moguri (Cambiar naipes), números HD** (27/10): el «2» salía sobre un bloque gris. Causa:
  `LoadDecimalDigitTiles` (mode_ms_top.c) dejaba que port_ppu dedujera fondo/tinta/sombra de cada tile por
  frecuencia; el 2 tiene más tinta que fondo → colores invertidos. Arreglo: todos los dígitos usan los colores del 0
  de su juego (gris o blanco). `test_moogle_charge_digits.py` (RED 71/71 → GREEN 40/39; ceros grises intactos).
  Mismo render: «Naipes activos 021/062» y total Moguri correctos.
- [x] **Cartas HD pequeñas (lista de naipes): encaje de la imagen**, #315 integrado `15f84f425`. Marcadas por el
  usuario: Simba (retrato desborda/cortado), Genio (asoma por la derecha), carta verde de objeto (cubo azul
  descentrado), carta gris de enemigo (sombra/Soldado). Hoja real de las105 revisada; recetas448 válidas. Pendiente captura en runtime final.
- [x] Tienda Moguri en vista de tienda (fila de 5 cartas con número): números HD se ven bien (captura usuario 27/10).
- [ ] **HUD HD (solo modo HUD HD; el HUD normal no se toca):** — tareas en la cola de arte (`hudhd-*`,
  `cartas-hd-encaje`, `escenas-retrato-x4`), registradas 27/10 para revisión en Atlas.
  Captura base 27/10: `build/hudshot` en khcom-hd-reload, ini del usuario (copia de `build/codex-evidence/
  quiver-crash-20261027`) con `hd_hud=1`, `fullscreen=0`, `KHCOM_WINDOW=1536x864`, `KHCOM_BATTLE=204:8:0`,
  `KHCOM_MONKEY=19:120`. Los números bajo el retrato y la pila del jefe solo salen al apilar/elegir: usar
  secuencia de combate con `STOCK`. Para 2.1 el usuario sustituyó Atlas por
  verificación directa del coordinador (AGENTS.md): capturar y corregir hasta
  comprobar, sólo modo HUD HD. #320 restauró los marcos; #341 revisa dígitos.
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

- [ ] **Galería de escenas sin vista previa**: en vez del texto «Sin vista previa», un fondo minimalista inspirado
  en KH (p. ej. corazón/corona/vitral) a baja opacidad, para que se vea pro (pedido del usuario 27/10).
- [x] **Cartas pequeñas HD centradas** (27/10): usan el recorte de su carta grande HD; PR abierto, hoja r2 en Atlas.

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
