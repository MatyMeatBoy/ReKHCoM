# Pendientes vigentes hacia la v2.0 (sintetizado 24/10/2026 ~19:00)

Única cola de trabajo, **resumida**. Todo el detalle, comandos y traspasos anteriores están en
**PENDIENTES_ARCHIVO_20261024.md** (esta lista al 24/10 ~19:00 completa) y en PENDIENTES_HISTORIAL_2026-10.md.
Método de depuración: DEBUG_WORKFLOW.md. Protocolo de la cola: COLA_COLABORATIVA.md. Estado vivo de piezas:
`python port/tools/art_queue.py --quiet list`.

## Actualización de estado — 25/10/2026

El usuario autorizó preparar y publicar una pre-release `v2.0-pre13`, y escribir a los reporteros de los issues
abiertos para pedirles que prueben; cerrar solo si confirman que quedó resuelto. El estado y los pasos de relevo
están en [HANDOFF_20261025.md](HANDOFF_20261025.md). Esta actualización reemplaza la nota anterior de esperar
a la v2.0 para responder issues.

- PR #90, `codex/release-v2.0-pre13`, prepara la versión y el changelog. Sigue abierto; no se ha compilado ni
  publicado la release. Issues #49, #77, #78 y #83 siguen abiertos y sin comentarios de sus autores.
- El usuario reporta tartamudeo al entrar por primera vez a la pantalla de título, además de blinkeo del Genio
  y espejo al alejar la cámara en la arena de Agrabah. La captura está preservada localmente en
  `build/user-reports/jafar-arena-20261025.png`; nueva tarea de cola pendiente `agrabah-jafar-arena`.
- El patrón inferior de la arena de Agrabah también requiere mejora. No integrar cambios visuales hasta subir
  una propuesta a Atlas y recibir aprobación de la versión exacta.
- “Añadir a Steam” escribió una entrada válida para la cuenta activa, pero el lanzador tiene un posible fallo al
  leer la cuenta desde `loginusers.vdf`. Steam y el juego siguen abiertos; no reiniciarlos durante las pruebas.

## Reglas del usuario (completas en REGLAS_DEL_USUARIO.md)

- Un solo integrador con token; rama y PR por entrega; nada de push directo de arte a main.
- Lo visual que necesite ojo humano va a la app de revisión (`import_image.py ... --parent <id>`, **imagen a la app antes
  del PR**); solo se integra lo aprobado ahí (con honores para el arte). Lógica aprobada: LOGICA_APROBADOS.md.
- La validación en hardware real no es pendiente: llega como reporte. Publicar (releases, comentarios) solo con OK.
- Nunca borrar carpetas ReKHCoM-vX ni ningún release. Trabajar solo en KHCoMPC (C:) y D:/01Claude/ReKHCoM-build.
- Cada PR de arte mejora LOGICA_APROBADOS.md. Toda falla o timeout de batería es un bug: reproducir.

## TRASPASO (25/10 madrugada, claude-integrador; el usuario duerme y pidió decidir solo, sin preguntar)

- Todos los PR integrados (#70-#89; #71 cerrado con la mejor solución). Cola online al día; permiso liberado.
- En main desde el último traspaso: Jafar sin espejo del genio ni sprites duplicados ni parpadeo del recuadro;
  Riku hereda el arte aprobado de los fondos que reutiliza (192→061, 179/181/183→055, 186→044, 187/188→045);
  logros Re:CoM (lista propia, offline; ver §3); aviso en la elección de versión; check_sources ok.
- Copia de prueba interna **v2.0-pre12** (main 741196d7f): `jugar/ReKHCoM-v2.0-pre12` + «Probar ReKHCoM v2.0
  pre12.bat», partidas de la pre11, HD enlazado; humo 9/9. (pre10 y pre11 eran de otra sesión: sus carpetas intactas;
  sus zips sueltos en la raíz se sobrescribieron por error y se borraron: se regeneran desde sus commits.)
- **Issues: el usuario pidió esperar a la v2.0** para responder y cerrar. Borradores abajo (§1).
- En curso: APK de prueba x86 con EU/US/JP (`build/android-smoke-2510.log`) para repetir la batería Android US/JP
  (falló porque el APK instalado cambió a mitad y no traía la imagen US) y reproducir #83 en el emulador.

## 1. Espera al usuario (decisión, revisión o OK)

- **Integrados 24/10 por orden del usuario** (sin aprobación en la app; quedan para comentar en la app si hace falta): #70, #75, #76, #79, #80, #81, #82, #84, #85, #86, #87. HUD HD y 112/120 siguen esperando revisión.
- PR #71 cerrado 24/10: entró la regla de actores fuera de cámara (112/120) y la vista previa de 112/120; los defectos de
  arte (recuadro negro, escalones, hueco marrón) siguen en la cola de arte, 115-118 apagadas.
- **Respuestas a issues para publicar con la v2.0** (cortas, sin decir que no se probó en hardware):
  - #78: «No era un fallo: en el modo Re:CoM los logros oficiales esperan. Desde la v2.0 el modo Re:CoM tiene su
    propia lista (los mismos logros + los del modo, por dificultad), solo en tu equipo, y un aviso al empezar.» Cerrar.
  - #77: «Debería estar arreglado en la v2.0 (si la fuente HD no carga, se ve el texto original; además busca la fuente
    de respaldo en Bazzite/Fedora). ¿Puedes probar?» Cerrar al confirmar.
  - #49: respuesta del borrador anterior (arreglo del lanzador Linux/Steam Deck). Cerrar al confirmar.
  - #83: el cambio de aspecto durante Jafar está arreglado; el cierre al tomar la carta del jefe sigue en estudio.
- **Probar jugando la lógica GBA de 3 estados** (Sí / No (Mejorado) / No): voladores (`MapEnm01/04`) ya se desvanecen
  también (probado en arnés con el contacto desactivado: 7 desvanecidos completos).
- **Logros Re:CoM — arte en la app** (`f59d55cee2bc419c819f57ad107c79ae`): borde de filigrana dorado para toda la lista
  Re:CoM e insignias de los 7 propios (historia + 1-3 estrellas). Al aprobarse: embeber el borde (como badges_png.inc) y
  dibujarlo en el menú del juego, el aviso y el launcher.
- **Darkside 01 con honores:** receta en main; se activa para todos cuando se apruebe «Darkside · arena y contexto».
- Preliminar V2 de jefes (`KHCOM_ARENAS_V2=1`): Trickmaster, Oogie, Maléfica, Úrsula, Marluxia final esperan comentarios.
- Maléfica 02/03 esperan revisión; Maléfica arena aprobada con comentario general pendiente.
- Diseños que piden decisión: videos HD en Linux nativo, HUD HD a lo Re:CoM sin equivalente 1:1 (contador CARDS, título).
- **v2.0 estable solo con tu OK** (lista de verificación al final).

## 2. Cola de arte — un solo worker (`r4`), en serie

Piezas pendientes en la cola: 074, 087, 090, 095, 099, 102, 115, 116, 117, 118, Maléfica 01, Maléfica arena, Oogie arena,
Trickmaster arena, diario (entradas y subentradas), y al final 081 (necesita extender la ventana) y 097 (es lógica de
sprites, no receta). **057 y 061 liberadas con diagnóstico, decisión de tile-map mayor:**
- 057: donantes de pasto por mata completa (5 matas de 8 tiles) y período vertical del muro (`port_scene057_tiles.h`).
- 061: la nube 2 parece resuelta; la nube 1 ambigua; capturar a 3840 y comparar con la hija.
Ya integradas/cerradas: 044, 045, 053, 054 (#50), 055, 098, 104, 110 (#53), 121, 122, 124, Agrabah 107/108/109,
Guard Armor, Darkside 01-07 respondidas. Comentarios nuevos a 087, 090, 095, 099 y 102 siguen abiertos hasta responderlos.
Flujo del worker: WORKER_BRIEF.md (imagen a la app primero; `--sent` obligatorio para `pr_abierto`).

## 3. Funciones y código abiertos

- **Logros Re:CoM (en main, #78):** las partidas Re:CoM evalúan los 90 oficiales en `achievements_recom_eu.dat`
  (79 de mundos/historia/jefes piden toda la partida en Experto: la dificultad más baja usada se guarda por archivo en
  kh_com_recom.ini, bits 11-12) + 7 propios en `achievements_recom_extra.dat` (Sora/Riku por dificultad, la mayor
  desbloquea las menores; ambos en Experto). Nunca online; la lista oficial y la cuenta esperan en partidas Re:CoM.
  Menú del juego: Y/Tab alterna Oficiales/Re:CoM; launcher: pestañas. Arnés: `KHCOM_CREDITS=frame:movie` con
  `KHCOM_ACHIEVEMENTS_LIVE=1`. Falta solo el arte (§1).
- **Editor de tiles:** en main (app de revisión → «Editor de tiles»): escenas del cartucho, lápiz, cuentagotas, borrar, rellenar, bloques, voltear, capas, deshacer, guardar por revisión, exportar a Tiled. Falta: sprites.
- **Catálogo histórico:** 12 huecos en ARTE_HISTORICO.md; regenerar casos factibles con fixtures actuales (tareas
  `catalogo-*` de `claude-relevo`).
- **Botones táctiles:** falta verlos en el emulador Android y en la web (punto Android).
- **Elección de modo al primer arranque** con vista previa (en la app, Pre): probarla en Android; si no gusta, quitar el
  bloque `preview` de port_ui.c.
- **Android antes de la v2.0:** rendimiento en gama baja (línea `timing:`), tablets con el encuadre nuevo
  (LOGICA_APROBADOS), Jafar en el emulador (`test_android_scenarios.py --groups bosses`), «Logros pegado» (pedir
  `kh_com.log` si vuelve).
- Honores verificados 24/10: 53, 54, 55, 121, 122 (+ eventos 177/178) encendidos y GBA intacto en EU/US/JP (21/21).
- **Baterías:** consultar `run_batteries.py --status` en `khcom-claude-int`. Hechas: todo menos rooms eu, battles eu
  (no arrancaron) y Android US/JP menús (APK sin imagen US: repetir con el APK nuevo); luego `--hd`. `debug_kit.py report` sin sitios nuevos.
  Caso skip JP `1_039` (0xC0000135) fue de las copias duplicadas del coordinador, no del juego: repetirlo solo.
- **Limpieza** de restos de pruebas en C: y D: (quitar enlaces antes de borrar; nunca carpetas ReKHCoM-vX).
- **Cuando haya cola en la app** (`COLA_PARA_AGENTES.md`, turno «agente»): responder cada envío con una versión hija.

## 4. Esperando reportes o datos (no se trabaja hasta que lleguen)

- #42 «pistas HD intercambiadas» (pedir nombres si reabre); Proton estable (pedir `kh_com.log`).
- Cara HD de Sora con el pelo cortado al recibir daño; cuadro negro en el combate de Agrabah con HUD HD (falta captura).
- Vita: undub por cabecera y velocidad. Switch #35: rendimiento (después de la v2.0).
- Reportes sin plataforma/dispositivo/versión: preguntar al reportero en el issue.

## 5. Después de la v2.0

- **Switch fuera de la v2.0** (motor nuevo hecho en SWITCH_MOTOR.md; vuelve con medida en consola real). Vita/PS4 a la
  GPU (RENDIMIENTO.md). PPU de GBA en varios núcleos para 60 fps a x1.
- Online (~70 %, ONLINE_ESTUDIO.md): prueba entre dos personas, relay en VPS, rollback y lobby en Android, sockets Switch.
- Menús de Re:CoM en HD (HUD_HD_RECOM.md), fondos de menús, números de cartas HD vs Re:CoM, más assets, datos para el
  modo Re:CoM, rutas completas de Sora y Reverse/Rebirth (#39).
- Mejoras futuras de lo «aprobado con mejoras» (no son pendientes de ahora).

## 6. Lista de verificación antes de publicar la v2.0 (no saltear)

- **Sin Switch en la v2.0:** sacar el `.nro` de la publicación.
- `test_update.py` con el paquete nuevo contra `respaldos/releases-estables` y `--olds respaldos/releases-prueba`.
- **No se borra NINGÚN release ni archivo**; publicar = solo agregar. README con la versión nueva. Copiar la estable a
  `KHCoMPC/respaldos/releases-estables` (mismo SHA).
- `release_check.py v2.0` antes y `--post` después; `check_sources.py`.
- Builds desde el commit final: Windows, Linux, Android, PS4, Vita, PS5 (VM), web; ningún paquete viejo.
- Combate de prueba con cada ejecutable de PC (`KHCOM_BATTLE=10:4`, `build/smoke_pc.sh`), sin `bios:`/CRASH.
- `test_hd_audio.py`, `test_hd_audio_flow.py`, `test_frameskip.py`, `test_hd_scenes.py`, `match_hd_scenes.py --check`,
  videos HD, `test_launcher.py` (sin `SDL_VIDEO_DRIVER=offscreen`).
- Copia de prueba para el usuario + `.bat`; notas cortas que anuncien el mod HD; nunca decir en público que algo no se
  probó en hardware.

## 7. Cerrado en esta síntesis (ya no son pendientes)

- 24/10 noche: lógica GBA «Mejorado» también desvanece a los voladores (MapEnm01/04), probado en arnés (7 fades sin crash). Windows: prioridad alta, timer 1 ms, sin EcoQoS y clase MMCSS «Games» contra tirones al cambiar de ventana (falta el reporte del usuario jugando).

Cuelgue Guard Armor 008/010 (era del arnés, resuelto); calidad HD (se queda «Buena»); vía GPU del importador (en main);
savestates en Trucos (#65); orden del mod HD (#63) y carga que conserva lo cargado (#66); launchers de Android y web
iguales al de PC; `test_text_options.py`; repo autosuficiente; paneo con zoom (#61); catálogo histórico cargado (#57);
lanzador Linux/Steam Deck (#58); botones táctiles en disposición móvil (#59) y primer arranque panorámico (#60);
lógica GBA de tres estados; cola de workers con imagen obligatoria antes del PR; cola limpiada (workers r2 sin trabajo
recuperable). Builds internas `v2.0-pre9` y `pre10` (no publicadas).
