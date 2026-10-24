# Cola vigente (21/10/2026, reparto con Codex)

Única cola de trabajo; los demás .md son evidencia. Lo terminado sale de aquí (queda en git y en CHANGELOG.md).

Reglas del usuario:
- Claude y Codex commitean y pushean a main; si uno lleva más de 10 min sin commits ni archivos tocados, el otro toma
  sus puntos anotando hora y evidencia aquí.
- Todo lo visual se entrega como copia jugable + .bat y espera aprobación del usuario; lo aprobado va a
  LOGICA_APROBADOS.md; método en PRUEBAS_VISUALES.md.
- La validación en hardware real no es pendiente: llega como reporte.
- Nunca borrar carpetas ReKHCoM-vX sin que el usuario lo pida.
- Publicar (releases, comentarios en issues) solo con el OK del usuario.

## TRASPASO 24/10 tarde (Claude; quedan pocos tokens) — LEER PRIMERO

**Objetivo nuevo del usuario (24/10): el repo autosuficiente.** Cualquiera (otro modelo con la cuenta de GitHub del
usuario, sin nada de este PC) debe poder clonar, poner su ROM y seguir los pendientes replicando todas las pruebas.
Hecho el primer paso: `EMPEZAR_AQUI.md` (raíz) y `port/progress/REGLAS_DEL_USUARIO.md` (las reglas que vivían solo en
la memoria local de Claude). Falta: revisar que cada herramienta que nombra funcione sin rutas de este PC (buscar
`D:/01Claude` y `KHCoMPC` en port/tools y volverlas parámetros con valor por omisión relativo). Antes era: `EMPEZAR_AQUI.md` corto (qué leer, cómo compilar, dónde van las ROMs, cómo correr cada batería y
herramienta, qué se instala y de dónde: toolchain llvm-mingw, SDL3, emsdk, Android SDK, devkitA64/VitaSDK/OpenOrbis por
Docker o VM). Instrucciones de las VMs/toolkit como texto (TOOLKIT_CONSOLAS.md), **nunca ISOs, discos de VM ni
binarios grandes en git**; todo lo sintetizado < 50 MB. Las rutas de este PC (D:/01Claude, KHCoMPC/...) en los .md
deben quedar como "carpeta de trabajo" con su equivalente relativo. Revisar el tamaño del repo (`git count-objects -vH`).

**Pedido nuevo, ANTES de la v2.0 (usuario 24/10): launchers de Android y web iguales al de PC** en diseño y
funciones (pestañas Inicio/Gráficos/Sonido/Mejoras/Controles/Editor de partidas/Logros, tarjeta HD, modos gráficos,
partidas/bancos, logros, enlace en red), fluidos y sin bugs (depurar).
- Relevado: **web** ya tiene las 7 pestañas (`port/web/index.html`, settings.js, editor-ui.js, achievements.js):
  comparar a fondo con el de PC (estilo: blanco, barras grises, cursor rojo, fuentes KHMenu; opciones nuevas como
  modo gráfico, borde GBA clásico, cartas/HUD HD, compresión) y completar lo que falte.
- **Android** es una lista simple de 253 líneas (`port/android/.../LauncherActivity.java`): reemplazarla por **el
  mismo launcher de PC** (`port/decompui/decompui.cpp`, ImGui + SDL3, ya en la APK) compilado para Android. Separar lo
  de Windows: SHA-1 (BCrypt → sha1 portable), diálogos (SDL_ShowOpenFileDialog, que en Android usa el selector del
  sistema), lanzar el juego (Intent a GameActivity por JNI en vez de CreateProcess), abrir URLs (SDL_OpenURL), sin
  importador de Re:CoM (Media Foundation: Android copia la carpeta hd armada en PC, como hoy), sin accesos directos.
  Táctil: botones grandes, desplazamiento con el dedo (ImGui con eventos táctiles de SDL). Probar en el emulador
  (capturas con `screencap`, flujo ROM → jugar → volver) y en el arnés de PC (`DECOMPUI_SHOT`).

- **Android: "Nueva partida Riku" en el título** (usuario 24/10, en su teléfono). En el código solo aparece con el
  truco `cheat_unlock_riku` (apagado por defecto, nada lo enciende en Android/APK/launcher) o con la historia de Sora
  terminada en la partida (`GAME_FLAG_SORA_CLEAR`, p. ej. una partida importada del PC). Comprobar en el emulador con
  instalación limpia que el título muestre solo la partida normal; si aparece Riku sin truco ni partida, es bug.

- **Elegir modo al primer arranque** (usuario 24/10, visto en Android): ofrecía "Modo HD" sin tener los assets
  importados. Sin el mod HD: no ofrecer el Modo HD (como mucho, el texto HD dentro de GBA mejorado); ofrecer siempre
  "GBA mejorado" y "GBA original/clásico". Agregar a cada opción **esquemas/ejemplos breves** (cartelitos con una
  captura chica: resolución y vista ancha vs GBA, mejoras) para que se entienda cada modo. Es visual: entregar copia
  + .bat y esperar aprobación. Código: `port_ui.c` (WIZ_MODE, sModeDesc*, `PortUiNeedsModeChoice`).
  **Hecho en parte (24/10):** sin nada del mod HD importado, la ventana va directo a "GBA mejorado / GBA clásico"
  y no deja volver a la que ofrece HD (`sModeNoHd`, port_ui.c). Falta: los ejemplos visuales y probarlo en Android.
- **Android, bugs del usuario (24/10), reproducir y arreglar antes de la v2.0:**
  1. Eligió iniciar **con textos HD** y al empezar **se desactivaron** (el asistente de partida nueva o el modo los
     pisa: revisar `PortApplyGraphicsMode` / asistente con hdText).
  2. Al abrir Opciones (**Start + Select**) y **bajar**, **la resolución cambió sola**: las primeras filas de
     Opciones (resolución/zoom) cambian con izquierda/derecha y el táctil o el stick las mueve sin querer. Pedido:
     **no poner las opciones de resolución al principio** de Opciones (moverlas más abajo o pedir confirmación).
  3. Después **se abrió la página de Logros sola y quedó pegado ahí** (sin salir). Posible: el mismo toque/stick que
     movió la resolución activó una fila que abre Logros, o la vuelta (B/Atrás) no sale de esa página en Android.
     Pedir al usuario su `kh_com.log` (Ajustes de la app → "Compartir registro") y el modelo del teléfono; reproducir
     en el emulador recorriendo Opciones con táctil y mando.
  Contexto: el teléfono del usuario es de **bajos recursos** y el juego va muy lento ahí; lo usa para probar la
  interfaz. Puede que los toques se registren tarde o repetidos con el juego lento: probar en el emulador con la CPU
  limitada (o cuadros saltados forzados, `KHCOM_SKIPTEST`) y revisar que un toque/tecla se tome una sola vez.
  Rendimiento Android de gama baja: también pendiente antes de la v2.0 (línea `timing:` del log ya disponible).
- **Calidad constante (hd_quality_probe.py, 24/10)**: en 4 escenas, RC=3 Q=70 por hardware pesa 23 % menos que Buena
  (151 vs 197 MB) con VMAF 95,4-96,1 (Buena 96,9-98,5); por software sale mal (VMAF ~52, descartado). Siguiente:
  probar Q 75/80/85 por hardware y elegir el que pese menos que Buena con su misma VMAF; la VMAF de ROP da 44 en
  todo (comparación desalineada en ese video: revisar el filtro antes de usarla).

**Hecho hoy (todo en main):**
- v2.0-pre3 y pre4 internas (PC); issues #40 #42 #45 cerrados (0 abiertos).
- Jafar APROBADO y encendido (`port_jafar.c`); Oogie y Marluxia en Preliminar V2 (`KHCOM_ARENAS_V2=1`), copia
  `pruebas/Probar Preliminar V2 jefes.bat`: **espera comentarios del usuario** (Trickmaster, Oogie, Maléfica,
  Úrsula, Marluxia; Darkside sus 7 poses).
- "Cuelgue" 32:9 = getenv por píxel → `PORT_ENV_ONCE` en todo el port (DEBUG_WORKFLOW.md).
- Launcher: "Comprimir lo instalado", sin 720p (ahorraba 6 %), adopta la instalación anterior, migra escenas al
  iniciar; juego: actualizar desde antes de los modos conserva opciones con "GBA mejorado".
- `test_update.py`: 7/7 desde 1.9.8..2.0-pre2 (incluidas las de Quiver). Quiver nos aceptó: **no borrar ningún
  release**; v1.9.10 y v1.9.8 republicadas (mismo SHA). Originales en `KHCoMPC/respaldos/releases-*`.
- README al día (v1.9.14). `scene_wide_review.py` arreglado.
- `timing:` en el log (consolas siempre, PC con `KHCOM_TIMING`): fase 0 del motor de Switch (SWITCH_MOTOR.md).
- **Switch fuera de la v2.0** (usuario): queda en la última pre; vuelve con el motor nuevo comprobado.

**En curso / sin terminar al cortar:**
- Pesos HD reales (`build/hd_sizes.sh`, en `build/hd_sizes.txt`): Fuente 6,7 GB, Buena 3,7 GB, Ahorro (q2) midiéndose;
  q5 ya no hace falta (720p quitado): cortar si corre. Poner el peso de Ahorro en `kScenesSize` (hd_import.inc).
- `hd_quality_probe.py` (calidad constante vs VBR, VMAF) corriendo: resultados en `build/hd_quality_probe.log` y
  `D:/01Claude/ReKHCoM-build/hdprobe/results.json`; si baja peso sin perder VMAF, usarlo en Buena/Ahorro.
- **Batería #12 del port de Vita: 209/209 combates EU sin fallo ni `bios:`** (`test_battles.py --stock`: guardar
  cartas L+R todo el combate recibiendo golpes). Ese crash no está en el nuestro. Cerrado.
- Dos baterías de combates JP duplicadas corrían (run_batteries.py): el usuario no dejó cortarlas.
- Jafar en el emulador Android: pendiente (correr `test_android_scenarios.py --groups bosses` con el emulador libre).
- Motor de Switch fase 1 (PPU en 3 núcleos del invitado): no empezado; plan en SWITCH_MOTOR.md.

## TRASPASO 24/10 (Claude, sesión nueva; usuario: "prerelease interna cerrando issues, limpiar pendientes, seguir
hasta el checkpoint de la Preliminar V2 de jefes y sus comentarios antes de la v2.0") — LEER PRIMERO

- **v2.0-pre3 interna (no publicada)**: Windows desde main cc3a9db9e (`build_all.py --platform windows --package
  --release-version v2.0`, zip copiado a `build/releases/ReKHCoM-v2.0-pre3-windows-x86.zip`). Copia del usuario
  `jugar/ReKHCoM-v2.0-pre3` + `jugar/Probar ReKHCoM v2.0 pre3.bat` (partidas de la 1.9.15 completo, hd enlazado a
  D:/01Claude/ReKHCoM-build/hd-completo). Combate de prueba EU/US/JP (`KHCOM_BATTLE=10:4`) sin crash.
- **Issues: 0 abiertos.** #40, #42, #45 cerrados con un comentario: el arreglo sale en la v2.0, pronto; #42 pide un
  issue nuevo con qué pista suena dónde si las pistas HD siguen cambiadas (el video no lo dice).
- **Oogie hecho** (Preliminar V2, `port/src/port_oogie.c`, `KHCOM_ARENAS_V2=1`): la sala como una bahía de un salón
  largo con sus propios tiles (columnas, ladrillos, guirnaldas, calabazas, escenario, baranda), sin la reja de
  dientes ni los contadores fuera de la bahía del combate; ladrillos y columnas hacia arriba; delante el campo verde
  en bloques espejados y corridos; abajo el camino sigue sus líneas y el antebrazo. Rectángulo GBA idéntico 6/6
  (16:9 y 32:9, `arena_before_shots.py --only oogie [--v2]`). Lógica en BATTLE_SCENERY_TECHNIQUE.md 9-11.
- **Jafar hecho y APROBADO** (usuario 24/10: "se veía genial"): `port/src/port_jafar.c`, encendido para todos
  (`KHCOM_JAFAR_WIDE=0` lo apaga). Antes no tenía vista ancha (búfer de pilares = global nativo en PC). GBA idéntico
  6/6 (16:9 y 32:9, `build/arena/jf_check.sh`). LOGICA_APROBADOS.md y BATTLE_SCENERY_TECHNIQUE.md 12.
- **"Cuelgue" de Jafar 32:9 resuelto**: era `getenv` por píxel (un cuadro por segundo). Patrón cazado en todo el
  port: `PORT_ENV_ONCE` (port.h) en DrawObjs (por fila, todas las escenas), patio de Guard Armor, bandas del título,
  HUD HD (por píxel), sprites HD, audio, daño, toast. Las 7 arenas en 16:9 y 32:9 terminan normal (242 s).
  DEBUG_WORKFLOW.md: clases nuevas (getenv caliente, búfer nativo, envoltura vertical).
- **v2.0-pre4 interna (PC)**: main 0bb66e76a, zip `KHCoMPC/ReKHCoM-v2.0-pre4-windows-x86.zip` (sha f7ea1853...) y copia
  `jugar/ReKHCoM-v2.0-pre4` + .bat. Humo oculto `build/smoke_pc.sh <copia>`: EU/US/JP combate 10:4 y Jafar 16:9/32:9,
  9/9 sin fallos. Imagen para el usuario: `pruebas/Jafar final (16x9 arriba, 32x9 abajo).png`.
- **Vita del otro port (ggdrn/KH_COM_VITA), concepto de rendimiento**: el juego compilado nativo para ARM (sin
  imagen invitada), lógica en el núcleo 0 y la PPU repartida en los núcleos 1 y 2 (3-6 ms por cuadro), audio en el 2,
  escala por GPU (vitaGL), 282 px de ancho; kubridge para los accesos a NULL. Nuestra Vita/Switch corren la PPU en el
  invitado, sin hilos: la idea a probar es **llevar las filas de la PPU al host** (que sí tiene hilos), como
  PORT_PPU_THREADS en escritorio. Su registro por segundo (logic/render/present en us) es buena idea para medir en
  hardware: agregar uno igual a nuestro log en consolas.
- **Pista de su #12 (EU, crash al recibir daño mientras se guardan cartas)**: acceso NULL (addr 0x10); probar el
  mismo caso en nuestro EU (PC "bios:" + Android 64) con la batería; si aparece, arreglarlo y buscar parientes.
- **Marluxia final hecho** (Preliminar V2, espera aprobación, `KHCOM_ARENAS_V2=1`; `port/src/port_marluxia.c` + ganchos
  de presentación en bos_lst.c / bos_lst_fld.c): vista ancha toda la pelea (antes saltaba de tamaño al prenderse
  BG2), el fondo de pantalla envuelve como en el hardware en sus 4 modos, el cuerpo (BG1) una sola copia y entero
  (el mapa antes de que el juego borre sus filas fuera de pantalla). GBA idéntico 6/6 (`build/arena/mx_check.sh`,
  `KHCOM_NO_MARLUXIA_FIELD`).
- **Launcher: "Comprimir lo instalado"** (pedido del usuario 24/10) hecho: HD_MOD.md "Comprimir lo instalado".
- **Peso real medido: 1080p Buena = 3,7 GB** (escenas 3,1 GB, videos 0,5, música 82 MB, sprites 24 MB; 21 min).
- **Pesos reales del mod HD por calidad**: `build/hd_sizes.sh` corre la importación completa en 1, 4, 2, 5 (resultados
  en `build/hd_sizes.txt`); pasar la tabla real al usuario y a HD_MOD.md / kScenesSize del launcher.
- **720p quitado del launcher** (usuario 24/10: ahorraba 6 %, menos del 20 % pedido): quedan Fuente / Buena / Ahorro a la
  resolución de origen; calidades viejas 3..5 valen como 0..2. Sigue: calidad constante (`hd_quality_probe.py`, VMAF)
  para bajar el peso de Buena/Ahorro sin perder calidad; medir Ahorro real (hd_sizes q2) y poner su peso en kScenesSize.
- **Migración al actualizar: hecha (24/10)**. `port/tools/test_update.py --new <paquete> [--olds <zips>]` instala cada
  estable vieja (y la v2.0-pre2, la que da Quiver) con ROM, partida, una opción cambiada y el HD en la estructura vieja,
  actualiza como un jugador o Quiver (que extrae y mezcla encima, sin borrar: leído en su código) y comprueba partida,
  opción, ROM, escenas y que el juego arranque: 7/7 (1.9.8, 1.9.9, 1.9.10 con carpeta; 1.9.12, 1.9.14, 1.9.15-pre1 (la
  verificada en Quiver), 2.0-pre2 (la que carga Quiver) planos). Zips de prueba en `respaldos/releases-prueba`.
  Arreglado en el launcher: adopta la instalación anterior si la nueva quedó al lado (zips hasta 1.9.10 con carpeta:
  copia partidas/opciones/ROMs/estados, mueve el HD, nunca pisa), escenas hd/movies → hd/scenes al iniciar (antes
  solo al abrir Sonido). Juego: quien actualiza desde antes de los modos y elige "GBA mejorado" conserva sus opciones.
- **Quiver** (usuario 24/10): "no me deja elegir cuál versión probé". Causas nuestras halladas: (1) borramos cada
  estable vieja al publicar la nueva (regla del 20/10), así que "Versions → Change Version" (Quiver 3.4.2+) no tiene
  entre qué elegir; (2) el README de main dice v1.9.13 estable (nunca lo fue; la estable es v1.9.14) y Quiver muestra
  el README en "How it runs"; (3) la retención del repositorio (#182 en tgeorgiadis/quiver-launcher) sigue abierta, sin
  respuesta de los mantenedores. Para la v2.0: no borrar más las estables (solo las de prueba), README al día, mismo
  nombre de zip de Windows siempre; preguntar en #182 si la retención sigue (publicar en el issue: con OK del usuario).
  **24/10: Quiver nos aceptó** (retención levantada). Regla: no borrar ni reemplazar ningún release ni archivo.
- **Motor nuevo de Switch** (usuario 24/10: "prepárate para implementarlo"): plan en SWITCH_MOTOR.md (fase 0 medir en
  la consola con una línea de tiempos por segundo; fase 1 PPU en 3 núcleos del invitado: hilos por el puente y
  contexto por núcleo en vez de TLS; fase 2 GPU solo si hace falta).
- Siguiente: copia jugable de la V2 y esperar los comentarios del usuario
  (checkpoint antes de la v2.0).

## TRASPASO 23/10 noche (Claude; /goal del usuario: "acabar y depurar todos los pendientes hasta llegar a la 2.0") — LEER PRIMERO

**Cómo seguir:** la cola del usuario está en "COLA del usuario" (abajo). Cada pieza: probar, subir a main enseguida,
anotar aquí (qué, commit, cómo se probó, qué falta) y escribir la lógica aprendida en el .md de su tema
(BATTLE_SCENERY_TECHNIQUE.md para arenas, ONLINE_ESTUDIO.md + GBANoCable para el online). Builds de lanzamiento de la
v2.0 solo con el OK del usuario.

**Estado:** escenas HD 97/97 y 97/97 (solo videos); baterías de escritorio relanzadas (`run_batteries.py --detach`;
"battles eu" falló al no existir build/pc-eu/khcom.exe: ya compilado, relanzar ese paso al terminar la pasada);
`debug_kit.py report`: el único sitio (evento 074, puntero basura en PlayEventCharaAnimSounds) arreglado con BUS_PTR.
Online: rollback + lobby + indicador de red + abandonar + demora manual en main; falta Android real y sala privada por
código en el juego. Arenas V2: Trickmaster, Maléfica y Úrsula hechas (apagadas, KHCOM_ARENAS_V2=1); faltan Oogie,
Jafar, Marluxia final, Darkside (revisión de poses del usuario) y la copia jugable de la V2.
**Herramientas nuevas:** `port/tools/arena_before_shots.py` (antes/después a x3), `netlink_pair.sh` (OUTSHOTS,
LINK_ENV), `netlink_lobby.sh`, `netlink_android.sh`, `netlink_cleanup.ps1`, `KHCOM_ROLLBACK_TEST`,
`KHCOM_NETLINK_GIVEUP`, `KHCOM_LOBBY(_KEYS/_SHOTS)`.

## TRASPASO 23/10 tarde (Claude, sesión nueva; el PC se reinició a mitad) — LEER PRIMERO

**Hecho y en main esta sesión:** carteles HD de World Cards / World Warp en US (19/19 US) y VER BARAJA de Riku quieto;
casos `rikudeck`/`premire` en test_hd_menus; Discord no se conecta en corridas de prueba; video HD: si Media
Foundation no da imagen (Proton sin H.264) queda la escena del cartucho; **online**: puente UDP (Android 64), permiso
INTERNET y ajustes de enlace en la app, modos Relay en el launcher de PC, **lockstep estricto** (ver ONLINE_ESTUDIO.md
"Lockstep estricto"), y el NULL del cartucho en `card_battle_riku.c` cardbattle_0 (rival en la VS). PC↔PC directo y
relay, con y sin lag/jitter/pérdida: mismo hash, semilla recibida, 0 esperas sin lag (demora 3).
**En curso al escribir esto:** prueba PC↔Android 64 en el emulador con el lockstep (`sh port/tools/netlink_android.sh
emulator-5554 12000 2600`, logs en build/netlink). Antes del lockstep daba DESYNC y crash; el crash ya está arreglado.
**Cortado por el reinicio:** las baterías de escritorio (events jp, wide eu...): relanzar con
`py -3 port/tools/run_batteries.py --detach --resume` cuando el emulador esté cerrado (poca RAM).
**Proton (usuario 23/10):** en builds tempranas "solo cargaba con Proton Experimental". Sin issue abierto; sin
equipo para probarlo. Pedir a quien juegue en Deck/Linux que pruebe la v2.0 con Proton estable y mande kh_com.log.
**#42 (23/10 noche): ARREGLADO en main (2df4e1d34)**, comentado en el issue. Causa: la escena HD se ofrecía una vez
por tarea de evento (`sPortHdSceneWork`), y un evento que sigue a otro (072, el último combate → 073, la sala de la
cápsula) reusa la misma dirección: la escena del final nunca se pedía. Ahora una vez por instancia (se olvida en
Event_2 y compara el número de evento). Los créditos además reproducen antes la escena del final si ningún evento lo
hizo. Probado con `eventplay:72` (flujo real, no el menú de depuración, que es lo que probaba test_hd_scenes):
zz_event_151 una sola vez y luego RSE, con y sin escenas HD. Esto pudo afectar a cualquier escena de un evento
encadenado: rehacer test_hd_scenes con cadenas reales. **Pendiente de #42:** "pistas HD intercambiadas" (solo hay un
video de YouTube; averiguar cuáles: pedir nombres al reportero si hace falta).
**Online Android con rollback:** conecta, pero el emulador es demasiado lento para llegar a la batalla con demora 2
(menús en lockstep): probar en un Android real (reporte) o con más tiempo.

## COLA del usuario (23/10 noche)

1. **Darkside: revisión de las 7 poses** (`pruebas/Darkside poses (elegir cuales van y cuales mejorar).png`, rama
   `claude/arenas` 744d8fadc, worktree `D:/01Claude/arenas-wt`). Espera los comentarios del usuario. Lectura propia:
   1 de pie bien; 2/5/7 bien con costura leve; 3 pelo cortado y escalón en el hombro; 4 espejo rígido; 6 puñetazo
   con huecos y pedazos sueltos (rehacer).
2. **Preliminar V2: arenas de todos los jefes** (pedido del usuario, en esta sesión): todo con `KHCOM_ARENAS_V2=1`
   (apagado hasta aprobar), lógica en BATTLE_SCENERY_TECHNIQUE.md "Preliminar V2". Hecho y en main (23/10 noche):
   **Trickmaster** (ajedrez hacia abajo en su perspectiva, 3a830e2ff), **Maléfica** (salón por bahías, un solo
   dragón, 1a43939b2), **Úrsula** (bajo el suelo lo que trae el mapa, una sola Úrsula, lomo continuo, 1a07929b6);
   rectángulo GBA idéntico en todas. Falta: **Oogie** (sala angosta: pared, rejas y suelo a los costados), **Jafar**
   y **Marluxia final** (hoy en vista Original dentro de la ancha: habilitar su vista ancha y extender), **Darkside**
   (espera la revisión de las 7 poses), y armar la copia jugable `pruebas/ReKHCoM-preliminarV2` + .bat (selector de
   jefe, antes/después).
3. v2.0 para todas las plataformas: depurar, cerrar issues, lista de verificación de abajo; builds solo con su OK.

## ONLINE — avance (usuario 23/10: "ir haciendo % de avances"; metas de ping: 20-150 ms local, 150-250 bien)

| Parte | % | Estado |
|---|---|---|
| Transporte (UDP directo, relay por código, Steam Spacewar) | 90 | PC; Android 64 por el puente UDP; consolas sin red (stubs) |
| Determinismo / lockstep | 95 | PC↔PC y PC↔Android 64: 20/20 chequeos iguales |
| Rollback en la batalla | 80 | PC↔PC: 0 esperas a ~160 ms de RTT, 0 desync, revisiones y sincronía de tiempo; falta Android |
| Lobby en el juego (salas públicas, crear, unirse) | 70 | hecho con servidor propio (probado de punta a punta) y Steam (sin probar con dos cuentas) |
| Inicio de sesión | 50 | Steam: aviso y "Reintentar"; servidor: nombre del jugador (online_name); cuentas propias: VPS futuro |
| Servidor propio (VPS: relay + lista de salas) | 60 | `netlink_relay.py` lista salas públicas ('A'/'L'/'R'); falta desplegarlo en un VPS |
| Android online | 75 | permiso, ajustes, relay/IP/salas por servidor; rollback y lobby sin probar ahí |
| Launcher (PC) | 90 | Steam/IP/Relay/Salas (Steam o servidor) y nombre |
| **Total online** | **~70** | |

Carpetas de prueba actualizadas (23/10 18:35): `ReKHCoM-OnlineAlpha-TU` / `-AMIGO` con los exes EU/US/JP nuevos,
launcher y `netlink.cfg = lobby:steam`, LEEME-ONLINE.txt "alpha 2". Prueba local del lobby:
`sh port/tools/netlink_lobby.sh 2600` (servidor local, uno abre sala, el otro entra; capturas lobby_*.bmp).

**Lobby (pedido 23/10):** en "Batalla de enlace" con el online activo, una pantalla del port (estilo Opciones) con
las salas públicas, crear sala, actualizar y volver al cable; sin sesión de Steam, el aviso de iniciarla. Todo
detrás de un "proveedor de salas" (listar/crear/entrar) con Steam ahora y el servidor propio después (mismo
protocolo para Android y consolas, que no tienen Steam). Las salas llevan nombre, versión del protocolo
(NET_VERSION), región del cartucho y estado (esperando / en combate).

## TRASPASO 23/10 ~12:40 (Claude anterior) — referencia

**Cómo anotar avances (pedido del usuario):** cada avance va aquí en el momento (qué se hizo, commit, cómo se
probó, qué falta), y al cerrar una sesión se deja un traspaso como este: estado, cómo retomar cada frente, dónde
están las herramientas y qué no hacer. Subir cada pieza verificada a main enseguida.

**Online (prioridad del usuario):** alpha completa y subida: `src/sio/sio.c` (ranuras, nunca cortar, búfer
adaptativo, chequeo de desync cada 120 cuadros), `port/src/port_netlink.c` (UDP/relay; `netlink.cfg` o
`KHCOM_NETLINK`), `port/src/port_steamlink.c` (Spacewar 480, `steam_api.dll` en bin/), `port/tools/netlink_relay.py`.
Docs: `ONLINE_ESTUDIO.md`, `GBA_LINK_NETPLAY.md`. Carpetas de prueba en la raíz: `ReKHCoM-OnlineAlpha-TU` /
`-AMIGO` (Steam host/join, partidas EU/US/JP, LEEME-ONLINE.txt). Probar en local: `sh port/tools/netlink_pair.sh
<cuadros> <teclas> [direct|relay]` (dos procesos ocultos, `BUILD=build/pc-...`; `LINK_ENV` para
`KHCOM_NETLINK_LAG/JITTER/LOSS/DROP`). Repo público aparte: https://github.com/MatyMeatBoy/GBANoCable.
**23/10 tarde (sesión nueva): online en Android y relay en los launchers.** `port_netlink.c` usa 4 primitivas UDP:
sockets BSD donde el juego es nativo (PC, Android 32) y el puente del host donde es imagen invitada (entradas nuevas
`udp_open/udp_resolve/udp_send/udp_recv` al final de `bridge_list.h`, host en `port/ps5/shared/udp_host.h`: real en
Android 64 y en el host de prueba Linux; Switch/Vita/PS4/PS5 responden 0 = sigue el cable). APK con permiso INTERNET
y sección "Enlace en red (alpha)" en la app (Desactivado / IP alojar / IP unirse / Relay crear / Relay unirse; escribe
`files/netlink.cfg`). Launcher de PC: modos Relay (servidor, puerto, código). PC directo y relay probados tras el
cambio (batalla VS sincronizada). Falta: prueba Android 64 en el emulador contra PC por relay (10.0.2.2), prueba real
por Steam (usuario y su amigo), un relay público (VPS) para que el modo relay sirva sin montar uno, rollback,
sockets reales en Switch (libnx: `socketInitializeDefault` + `UDP_HOST_POSIX`).

**Issues:** #43 y #44 cerrados (confirmados). #42, #45, #46 tienen arreglo en v2.0-pre1 y esperan al reportero;
#40 sin datos. Releases: v2.0-pre1 (Win/Switch/Android), v2.0-pre2 (Win). La v2.0 estable solo con OK del usuario.

**En curso al cortar:**
- ~~Batería US de menús HD: 17/19~~ **hecho 23/10 (sesión nueva): 19/19 US**, EU 14/14 en world/warp/deck. En
  US/JP el cartucho deja de dibujar el cartel como sprite en el estado de selección (queda la copia BG):
  `mode_worldinspect.c` / `mode_worldwarp.c` dibujan ahí el cartel HD con `PortHdStillSprite` (como Ver barajas),
  alineado con la copia BG (visto a 1080p). `test_hd_menus.py` resuelve `--hd` a ruta absoluta (con una relativa
  todos los casos caían al cartucho). Partida US de prueba: `build/us_menus_test.sav`.
- Re-ejecución de `debug_kit.py run --suites events --versions jp` (se había cortado por recompilar a mitad; ya
  arreglado en case_bin.py). Ver `build/battery-events-jp-rerun.log`.
- Los "timeouts" del primer intento US no eran cuelgues: START en una escena abre la pausa de escena (detiene
  cuadros a propósito); el test copiaba la partida con el nombre EU. Arreglado (copia kh_com/_us/_jp.sav).

**"Nueva partida vuelve al título" (usuario 23/10, luego dijo que quizá lo imaginó):** no estaba anotado. Probado
EU con el mod HD completo y A: título → nueva → ROP HD → evento 0 → escena HD → selección de mundo, sin volver
al título. Variantes con START durante la intro (HD, GBA original, sin HD, primer arranque) : las 4 quedaron en la
intro porque el START del test abre la pausa de película (previsto), ninguna volvió al título. Cerrado como no
reproducible salvo nuevo reporte con pasos.

**Otros pendientes vivos:** Switch motor nuevo (deko3d / PPU multinúcleo) NO implementado, solo investigado (#35);
video HD en Linux (diseño GStreamer más abajo); Proton; pelo de Sora y cuadro negro en Agrabah (esperan captura /
savestate del usuario); números de cartas HD vs Re:CoM.

## Estado (21/10 ~19:15)

- **v1.9.14 publicada (estable, Latest)**, commit a97268171 (main): PC (Windows, Linux, Steam Deck) estable; Android,
  Switch, Vita, PS4, PS5 (PPSA98511.zip, etiqueta ps5-beta) y Anbernic como `-beta`. Incluye J, #37, #38, I (PPU en
  varios núcleos, 90 eventos sin MISMATCH), mod HD (beta). Android 64 con imagen movida: 37/37 ok. Comentado en #22,
  #25, #33-#38; #25 cerrado. Los demás se cierran cuando el reporter confirme. Releases v1.9.12 y v1.9.13-pre2: borrar
  (el clasificador bloqueó `gh release delete`; lo hace el usuario). Escenas Preliminar V1 de Codex sin aprobar: NO
  integradas (rama claude/integrate-20261021).


- **Publicado:** v1.9.12 estable (Latest), todas las plataformas + alphas de Switch y Vita; web en Pages.
  v1.9.13-pre2 de prueba (Windows, APK 32/64, Switch GPU) comentada en #22, #33, #34, #35.
- **Rama `claude/perf-android-reloc`** (no está en main; va a main cuando pasen sus dos baterías):
  - J, Android 64 en teléfonos donde 0x01000000 está ocupada (#22 Poco F8 Ultra, #34 Mate XT): la imagen se carga en
    otro hueco y se reubica. En el emulador con `KHCOM_IMAGE_MOVE=1` carga en 0xe2600000 y los menús salen bien.
  - Caja de error con "Share log" (modelo, Android, logs). Si el juego se cae, `kh_com_host.log` trae la señal y la
    cadena de llamadas del juego (direcciones del enlace: `llvm-symbolizer --obj=guest-<región>.macho`).
  - #37 (sin teclado no se salía de "Press..."): un botón del mando, un toque o Atrás cancelan la captura.
  - #38: "ocultar controles táctiles" con un mando conectado no dibuja nada, ni en menús (sin mando queda el botón
    de opciones). Solo Android.
  - I, PPU en varios núcleos (Windows): falta la verificación.
- **21/10 ~17:00, pausadas por espacio:** el emulador reservaba 9 GB y el pagefile creció a 35,7 GB (C: llegó a
  0,3 GB). Android (menús y fixtures con imagen movida: todo ok salvo el selector de eventos, ya arreglado) y la PPU
  (80/194, 0 `MISMATCH`) se retoman con `--resume` cuando haya RAM/disco; la de escritorio sigue. Reiniciar Windows
  devuelve el pagefile. Arreglados después: Event_0 (selector de eventos) y los dos sitios del Bosque de los 100 Acres
  (`BUS_PTR`, `port/src/port_bus.c`), vistos en el informe de escritorio (`scenes` ahora va primero en las baterías).
- **Baterías (estado anterior):**
  - Android, imagen movida, EU menús/fixtures/jefes: `build/android-move-battery.log`,
    `build/android-scenarios/report-eu-khcom_image_move.json`. Relanzar: el mismo comando con `--resume`
    (`build/move_cmd.txt`).
  - PPU: `KHCOM_PPU_VERIFY=1`, eventos US en vista 16:9 (`--wide`; en vista GBA no hay filas fuera de la imagen y la
    verificación no prueba nada): `build/ppuverify-us.log`, `build/ppuverify/us_wide/*/kh_com.log` sin `MISMATCH`.
  - Escritorio (`run_batteries.py --status`): eventos EU hechos, US en curso; luego JP, 16:9 y salas.
- **Protecciones nuevas (21/10):** `check_sources.py` (constantes SDL del shim de consolas con el valor de SDL,
  literales de Java cerrados) corre en `build_android.py` y en `release_check.py`; ya encontró `SDLK_DELETE` mal en el
  shim. El arnés Android ya no da "ok" a un caso que todavía no arrancó, cuenta como caída el `crash:` del host y marca
  `BAD-APK` si el APK de prueba se armó sin `--smoke-env KHCOM_FAST=1` (corría la batalla de prueba en cada caso).

## Estado 22/10 ~15:00 (Claude, sesión principal; usuario: "sigue con lo que queda y haz un debuggeo grande")
- **Hecho y en main hoy:** oclusión de piezas HD en la imagen y en la capa de pantalla (`MainHdOver`, `HdOverLayer`);
  cartas HD en el menú de barajas y la vista Original; **22 cartas de mapa HD** (también en la síntesis de estancias);
  **13 mundos HD** (`cl_###` recortados con la silueta GBA, grupo 4); borde 1 del GBA clásico (elegido por el usuario)
  con 6 cartas de mapa reales leídas de la ROM; dígitos de cartas = los de Re:CoM (la fuente de diálogos en cartas fue
  rechazada: solo HUD y diálogos). 190 piezas HD; el importador del launcher = Python (dif. máx. 1).
- **Arte HD 1:1 con GBA: ~95 %.** Falta: proponer al usuario los tentativos (medidores, contador, recarga) con capturas.
- **Depuración grande en curso:** batería de escritorio con HD encendido (`run_batteries.py --hd --detach`,
  `build/batteries-hd.log`, informe `build/debugkit-hd/report.md`); la de HD apagado terminó limpia (294 casos, 0
  caídas; resultados en `build/batt-hdoff-20261022`). Ganchos nuevos: `KHCOM_CONFIG="k=v;k=v"`, `KHCOM_HD_SPRITES`,
  `KHCOM_MODE=worlds:MASK`. Luego: Android 64 con HD e imagen movida; consolas contra el shim.
- Comprobado 22/10 ~15:00: cartas de mapa, mundos, combate y borde 1 con HD en EU/US/JP (vista Original y Auto) y en
  el build reubicable (`configure_pc.py --reloc`, como Switch/Vita): 0 fallos; `check_sources.py` ok (shim).
- **Usuario 22/10 ~15:30:** le gustan el anillo de vida de Sora (Re:CoM `pl_gauge`) y las cartas; pidió mejorar la carta
  de recarga → **hecho**: `com_card02` (carta negra con borde plateado, 1:1) y su contador 1..19 con los dígitos blancos de
  `card_no` (`gReloadCounterWhite`, 19 composiciones). El "píxel" junto a la guarda es la cadena del llavero del arte HD.
  **Anillo de vida: primera versión en main** (`PortHdHpRing`, `pl_gauge` sin la cara horneada, pista vacía gris,
  se vacía desde la punta de la barra y luego el anillo; la cara HD se corre a (14,15) para que entre). La lógica de
  Re:CoM vive en su ejecutable (el .RTM solo trae texturas: `UK_gauge.imz` tiene las piezas: cuartos de anillo gris/
  verde/negro, barra, dígitos, HP, MAX, D-POINT). **Usuario: igualar la lógica real del HUD de Sora de Re:CoM**
  (es compleja): buscar una implementación (mods, OpenKh, ingeniería inversa) y reimplementarla.
- **HUD de combate como el 1.5 PC (usuario 22/10 ~16:00):** dígitos de cartas de `Cockpit.imz` (los de Re:CoM, no
  `card_no`), contador CARDS de mazo restante, carta de recarga con su número, anillo con la lógica real. Hallazgos y
  plan en HUD_HD_RECOM.md ("HUD de combate de Re:CoM 1.5 PC"). Ajustar tamaños/posiciones de todo lo HD al original.
- **Decompilación positiva (22/10 ~16:40):** CPlayerGauge leído (HUD_HD_RECOM.md "Decompilado"). Sigue:
  `port_hdlayout.c` (lector/dibujante LAYD/SEQD en C, como el SequenceRenderer de OpenKh) y el medidor de vida de 1.5 con
  esa lógica; luego CCardRevolver (pila, contador CARDS, recarga). Prueba de juego del anillo actual: daño, cura y
  >280 ok, 0 fallos; audio: test_hd_audio 0/9, test_hd_audio_flow 7/8 (el título se cortó por tiempo: revisar).
- **22/10 ~16:30 (Claude que retomó):** medidor de vida de Re:CoM 1.5 en main (977f064cf, `port_hdlayout.c`). Inventario de los menús de Re:CoM (268 layouts, texturas en español, 28 clases decompiladas): HUD_HD_RECOM.md "Menús de Re:CoM". Sigue: un menú por vez con su lógica decompilada (orden propuesto: pausa, subida de nivel, naipe conseguido, guardar/cargar, elegir mundo, síntesis, campamento, tienda, diario, minimapa).
- **Issues abiertos (22/10 21:00, usuario: leerlos todos y replicar hasta arreglarlos):** #46 Android se cierra al
  ganar el primer combate del evento de Agrabah (US 124 ya se cortaba por tiempo en la batería: era este bug); #44
  efectos/voces mudos con la música HD (v1.9.15-pre1); #45 voces undub que suenan en inglés (ataque en el campo, 2º
  golpe aéreo) [**arreglado**: el mapa de Undub no tenía el primer instrumento del grupo de voces (SONG_VO_SR_ATTACK00
  seguía con la muestra inglesa); `check_undub.py` lo halla, `test_undub.py` compara las 83 voces EU/US con el JP: 83/83] [#44: **arreglado en main** (31cbfd92d, posterior a la pre1 del reporter); `test_hd_audio.py` ahora
  toca la música HD de verdad (`KHCOM_HD_MUSIC`; antes corría sin ella y pasaba sin probar nada) y suma punto de
  guardado, ataque en el campo y voces undub: 0/13]; #42 finales HD: solo créditos [**arreglado en main** (38ee801ce): con "videos HD" el importador copia también zz_event_151/621/631 y el juego los toca (IsEnding en port_hdmovie.c) aunque `hd_scenes` esté apagado; `test_hd_scenes.py --movies-only` 4/4, mod completo 97/97; respondido en el issue (reimportar videos)]; #43 Switch: colores en cartas superpuestas [las fotos muestran PALETAS mal (contornos rojos en Sora y los soldados, colores cambiados en Sora), no teselas; en PC la misma batalla (evento 095, soldados de cartas) se ve bien, sin paletas agotadas (registro temporal en LoadObjPalette: 0 fallos) y el salto de cuadros (KHCOM_SKIPTEST en combate) solo cambia las filas 0-10 durante el mosaico; el reporter usa la build de prueba v1.9.15-pre1 de #36 (sin mod HD). 23/10: con el salto de cuadros apagado se arregla (pero el audio va mal): es del salto. En PC, combate 095 con KHCOM_SKIPTEST, el único efecto era el mosaico de rotura de carta: las filas de arriba de la vista ancha (gy<0) empiezan a mitad de bloque y tomaban la referencia afín del último cuadro DIBUJADO; arreglado (MosaicBlockStart, port_ppu.c): 0/194 cuadros distintos con y sin salto. Los contornos rojos de las fotos no salen en PC ni en reubicable: si siguen, falta una build de prueba de Switch (pedir OK al usuario)]. Pedidos datos de equipo en
  #46, #42, #43. #40 arreglado (avisar al publicar). #46 (23/10): las cadenas evento→combate dieron 0 bios/crash; los fallos de "no llega" eran del arnés: bosskill golpeaba piezas intocables (Guard Armor: torso antes que las extremidades, enemyCount daba la vuelta a 250) y dejaba un knockbackLift viejo (un Bandido volaba miles de cuadros); arreglado en main.c (bosskill) + modo btllog. En Android los casos largos eran inválidos: el getenv del invitado (port/ps5/guest/src/libc.c, Android 64/PS5/Switch/Vita) cortaba cada variable a 159 bytes, así que KHCOM_MODE/KHCOM_KEYS perdían las pulsaciones y el evento se quedaba parado; ahora la ranura crece. Con eso, emulador Android UBSan US y EU: evento 107 → combate → 108 → campo → dos combates más hasta el cierre programado, sin crash; Jafar (113) ok. No se reproduce: falta el modelo/versión del reporter. Guard Armor (008) con bosskill sigue sin cerrar: la derrota espera BgFxIsActive() (chara.c, CharaObjUpdateDefeat) y un efecto de muerte de pieza no termina; investigar si es del arnés (todas las extremidades a la vez) o del juego.
- **Pendientes HD (22/10 noche, Claude; se van tachando):**
  - [x] TIENDA en alemán e italiano (22/10 20:35): MOGRY-LADEN y NEGOZIO MOGURI en HD (`--language de/it`, registro
    `KHCOM_HDDRAWLOG`).
  - [x] Icono de Monstruo en US/JP: fila propia `gWorldIconMonstroHDUs` (mode_worldinspect: 12 iconos HD, nada fuera
    de la caja).
  - [x] `check_hd_sprites.py` (nombres únicos, símbolos por versión, piezas dentro de la caja, tiles que el código
    nombra) corre en `check_sources.py`: 300 filas, 0 problemas.
  - [x] Registro `KHCOM_HDDRAWLOG` y batería `test_hd_menus.py`: **68/68 ok** en EU (22/10 21:00): cada menú
    en 5 idiomas + medidor a 3..100 %, comprobando que el HD sigue en el cuadro quieto (no solo al entrar).
  - [ ] Contador de mazo (`gRevCount*`) y medidor del enemigo como Re:CoM: buscar capturas del HUD de combate de
    Re:CoM en internet y reconstruirlos con Cockpit/gauge.lad y el código decompilado (CCardRevolver, CEnemyGauge).
  - [x] (23/10) BONUS ÉLITE y VER BARAJA de Riku vistos en el juego. VER BARAJA volvía al píxel al quedar quieto
    (copia BG, como el de Sora): arreglado en `card_deckmenu2_riku.c`. Casos nuevos `rikudeck` y `premire` en
    `test_hd_menus.py`: 5 idiomas EU + US, 12/12.
  - [x] US y JP: batería de menús US 19/19 (+ rikudeck/premire); JP no tiene carteles HD.
  - [x] Gemelo Python del dibujante: `port/tools/khhd_seqdraw.py`.
  - [ ] Batería HD de escritorio (`run_batteries.py --hd`): corría con el build anterior; repetirla con los cambios
    de hoy al terminar.
  - Sin equivalente en Re:CoM (se quedan GBA salvo decisión del usuario): TRANSFERT, BOUTIQUE MOG, REISEBERICHT y
    los informes de Riku del diario, entradas de la pantalla de título (faltan letras), iconos de bonus del nivel.
- **22/10 noche (Claude):** en main: títulos de menú HD en 5 idiomas (69 carteles), VER BARAJAS quieto
  (`PortHdStillSprite`), iconos de los 13 mundos (world_icon), **medidor de vida corregido** (a1be11b8c: bucle, barra,
  máscaras, layouts remasterizados). Todo en HUD_HD_RECOM.md. Sigue (necesita referencia visual de Re:CoM o decisión
  del usuario): contador de mazo a lo "CARDS", medidor del enemigo, entradas del título (faltan letras), "NUEVO" →
  "¡NUEVO!". La batería HD (`run_batteries.py --hd`) seguía corriendo a las 18:25 con el build anterior.
- Arreglado: las muescas de la corona dejaban ver la ilustración HD (se borra su franja superior, tapada por el marco).
- Propuesta original: `pruebas/HD tentativos (medidores contador recarga).png` (medidor de vida en anillo de
  Re:CoM, medidor del enemigo, contador CARD/S, números de recarga): no son 1:1, espera su decisión.
- **Espera al usuario:** Darkside (arenas), OK para builds v2.0.
- **23/10 madrugada (Claude):** v2.0-pre1 local para el usuario en la raíz (`ReKHCoM-v2.0-pre1`, `Probar ReKHCoM v2.0
  pre1.bat`): paquete de `package_release.py` (build/pc, pc-us, pc-jp reconfigurados), limpia (sin partidas ni ini),
  ROMs EU/US/JP y HD importado de cero con khhd_import.py --scenes + khhd_sprites.py (103 videos, música, 300 sprites,
  subtítulos). Baterías v2.0 desde cero (`run_batteries.py --detach`, build/batteries.log; build/pc-eu estaba
  configurado sin los .c nuevos y no enlazaba: reconfigurado). Después: `run_batteries.py --hd --detach`.
  Ojo: `python` en el PATH ahora es el venv de hermes (sin numpy): usar `py -3`.
- **23/10 ~03:30 (Claude):** carpetas del HD ordenadas (262fdb5c9): `hd/movies` (ROP, RSE, RRE), `hd/scenes`
  (todas las zz_event, también las de los finales), `hd/music`, `hd/subs`, `hd/sprites`. El launcher de PC mueve
  las escenas de una importación vieja (renombrar), el de Android copia también `scenes`, el juego busca primero en
  `hd/scenes` y si no en `hd/movies`. test_hd_scenes: nuevo 97/97 (Re:CoM 97/97, finales 3/3), viejo 97/97
  (`hd-completo` queda con el orden viejo a propósito: prueba el respaldo). La pre1 de la raíz ya tiene el orden
  nuevo y el programa con este cambio. Espacio: borradas pruebas viejas (eventtest, batt-hdoff, ppuverify, mx_*,
  las de Codex del 18/10): C: de 13,9 a 25,3 GB libres; se guardan codex-evidence y preliminar-v1 (revisión de arte).
  Baterías v2.0 corriendo (scenes EU/US/JP ok; sigue events).
- **Comentarios del usuario sobre la v2.0-pre1 (23/10, en orden; se van tachando):**
  - [x] **Bug (arreglado ac3e2c4f1 + hermanos):** abrir Opciones en la transición de una escena HD al juego y volver dejó la pantalla negra con la
    pausa de escena abierta e invisible (memoria del proceso: modo Event, sScenePause=1, sOpen=0, sReturn=0); START la
    cerró. El evento abre la pausa al soltar un toque corto de START (event_message.c UpdateEventSeq).
  - [x] Logo HD del título con líneas finas (arreglado y visto a 1080p): bordes de piezas en medio píxel + la inclinación de 0,01 px
    (port_hdsprites.c Quad); arreglo hecho (esquinas a píxel entero), falta verificar en el título estático.
  - [x] (3de66108c) "Elige un mundo": la imagen grande del mundo se ve mal (máscara del sprite GBA, bordes pixelados); en
    "Naipes mundo" los iconos HD se ven bien: usar el mismo arte/transparencia, o volver al pixel si no se puede.
  - [x] (cartas chicas, grupo 6) "Naipes mapa": la carta grande ya es HD; las chicas de la grilla siguen en pixel: hacerlas HD.
  - [x] (cartas chicas, grupo 6) "Estado": las cartas de aliados (Donald, Goofy) siguen en pixel: hacerlas HD.
  - [x] (3de66108c, mismas filas) Diario (Historia): la imagen del mundo es la misma versión mala que "Elige un mundo" (mismo arreglo).
  - [ ] Opcional: mejorar los fondos de esos menús (p. ej. "Naipes mundo").
  - [x] (estilo 12) Diario > Historia: el submenú no tiene las mejoras de vista ancha de los demás (el usuario marcó el margen
    izquierdo, el borde derecho y la barra de arriba a la derecha que se corta): dejarlo como los otros submenús.
  - [x] (3de66108c) Imagen de mundo HD (Elige un mundo, Diario): reemplazar las feas por el arte de "Naipes mundo" que se ve bien.
  - [x] Anillo de vida HD: bordes lisos (antialias) en la cara y el final de la barra. (supersampleo 4x, 2x consolas/Android; no en el renderer software)
  - [ ] Números de las cartas HD quizá muy chicos: comparar con Re:CoM (referencias/capturas del HD).
  - [ ] Cara HD de Sora al recibir daño: el pelo sale cortado. (23/10: la textura de caras de Re:CoM está entera, sin cortes en sus bordes ni máscaras en la secuencia;
    el sprite de combate se ve normal en los cuadros del golpe: falta una captura del momento exacto.)
  - [x] (23/10: títulos Quitar/Añadir naipes en HD, 5 idiomas EU + US; JP sigue GBA) Editar baraja (Quitar/Añadir naipes): al cambiar con L/R los textos HD aparecen un cuadro antes del fundido;
    el número de la carta grande parpadea al cambiar de carta; el título "QUITAR NAIPES" en HD como los demás. (El título "QUITAR/AÑADIR NAIPES" es BG, no sprite: hacerlo con PortHdStillSprite y un cartel armado desde el
    BG; los demás carteles salen de sprites con hd_menu_titles.py.)
    23/10 hecho: el título está en BG3 (filas de arriba de gDeckRemove/AddGridMap con los tiles de texto del
    idioma); hd_menu_titles.py BG_BANNERS arma el cartel (Q = O + cola, Ñ = N + virgulilla) y card_deckmenu2.c lo
    dibuja con PortHdStillSprite(tiles de texto, mapa). US: mismas recetas que el inglés, con gDeckMenuTiles (compila; no visto en pantalla). JP sigue GBA.
    Antes: no es OAM (volcado OAM de la pantalla Quitar naipes: nada arriba a la izquierda) ni las
    filas de arriba del mapa de BG3 (gDeckRemove/Add/DeleteGridMap, renderizados con gDeckMenuTiles + texto por
    idioma: vacíos ahí). Queda mirar BG0-2 (paneles gDeck1/2/3Panel). Re:CoM (FO0100 SP_camp) trae "AÑADIR NAIPES" /
    "QUITAR NAIPES" solo en letra recta chica; la cursiva de los carteles no tiene Q ni Ñ: armarlas (O + cola,
    N + virgulilla) o escalar la letra del cartucho. Es visual grande: mostrar al usuario antes de cerrar.
  - [x] (nivel en vivo; sin cortes medidos) Pequeños cortes de audio al cambiar de menú: la música HD sin fundido de entrada/salida como el juego;
    revisar la lógica general del mod HD y depurar. 23/10: test_hd_audio_flow con el recorrido de menús de pausa
    (pausemenus) 0 vacíos y 0 saltos; el nivel de la pista HD se aplica ahora en vivo en el stream (antes iba en las
    muestras 0,2 s adelantado: los fundidos llegaban tarde y a saltos). Si el usuario sigue oyendo cortes, pedir dónde.
  - [x] (launcher, Media Foundation; 23/10) Importar HD con calidad a elegir: resolución 1080p (sugerido PC) / 720p
    (sugerido Android) × compresión Fuente / Buena (sugerido) / Ahorro. H.264 High + VBR + B por el encoder de
    hardware (ganó a Main/CBR y al de software). VMAF contra la fuente: Buena 1080p 94,9 (apertura) / 97,1 (escena);
    720p Buena 90,5 / 97,1; Ahorro 82-91. Reproducidos en el juego: 30 fps, 0 cuadros tarde. Tabla en HD_MOD.md.
    Falta: el importador del APK (Android) solo copia; recodificar en el teléfono no (usar el del PC).
  - [ ] Linux nativo sin videos HD (port_hdmovie.c: solo Windows y Android). Steam Deck con Proton usa la de
    Windows. Diseño (23/10): GStreamer por dlopen (API de funciones, sin structs salvo GstMapInfo): pipeline
    `filesrc ! decodebin ! videoconvert ! video/x-raw,format=NV12 ! appsink` + `audioconvert ! audioresample !
    audio/x-raw,format=S16LE,rate=48000,channels=2 ! appsink`; reutilizar de PlayFile (Windows) el ritmo por marcas
    de tiempo, subtítulos, pausa y fundidos sacándolos a funciones comunes. Probar en la VM ARM64 con
    gst-plugins-good + gst-libav (H.264/AAC). FFmpeg descartado: sus structs cambian por versión.
    23/10 (revisión): el paquete de Linux es i386, así que un dlopen de GStreamer necesita las librerías **i386** del
    sistema (gstreamer1.0-libav:i386), que casi nadie tiene (en Steam Deck tampoco: ahí va la de Windows por Proton).
    Alternativa que no depende de la arquitectura: lanzar el `gst-launch-1.0` (o `ffmpeg`) de 64 bits del sistema
    como proceso aparte y leer por tubería los cuadros (RGBA/NV12) y el audio (s16le 48 kHz), con el mismo reloj por
    audio que PlayFile. Si no hay ninguno instalado, la escena del cartucho (como Proton sin H.264). Decidir antes de
    implementar (es diseño grande): pedir OK al usuario.
  - [ ] Proton (usuario 23/10): que la versión de Windows corra bien en Proton estable sin "Experimental"; revisar
    reportes (issues) de Steam Deck/Linux y qué falla (Media Foundation de los videos, D3D11, audio).
    23/10: ningún issue de Proton/Deck abierto (solo #19 GLIBC, cerrado). Riesgo principal = H.264 por Media
    Foundation en Proton estable: ahora el video HD lee su primer cuadro antes de mostrar nada y, si no sale imagen,
    el juego sigue con la escena del cartucho (antes: video negro con sonido). test_hd_scenes 97/97 sin cambios.
    Falta: probarlo en Proton real (llega como reporte).
  - [ ] Ripear más assets aprendiendo de otros mods (MelonMIX HD y similares, "voltron" de repos): qué assets de
    Re:CoM sirven y cómo los usan.
  - [ ] v2.0: pasada de rendimiento para todas las consolas, solo lo medible y sin riesgo (medir antes/después,
    pruebas píxel a píxel).
  - [x] (launcher, 23/10) Instalador: barra de progreso al importar/recodificar el HD (avanza dentro de cada
    video); "Instalar en otra carpeta..." (tarjeta Partida: copia todo a <unidad>:/Games/ReKHCoM o la elegida, carpeta
    vacía, nunca borra la original; probado byte a byte); "Añadir a Steam" (Inicio: juego externo en shortcuts.vdf de
    la cuenta más reciente, con portada y logo; actualiza sin duplicar, las demás entradas intactas, copia
    shortcuts.vdf.rekhcom-backup; probado sobre una copia del Steam real, no sobre el real).
  - [~] (estudio hecho 23/10: port/progress/ONLINE_ESTUDIO.md; cable por red a nivel de paquete, alpha completa: Steam/IP/relay,
    batalla sincronizada, sin cortes, búfer adaptativo, interfaz en el launcher; carpetas de prueba
    ReKHCoM-OnlineAlpha-TU / -AMIGO; falta la prueba real entre dos personas y lo de "Siguiente" en ONLINE_ESTUDIO.md) Online (estudio preliminar + alpha): llevar el modo cable link (espera latencia cero) a online moderno
    manteniendo su lógica; rollback si aplica; "voltron" de lógica de juegos online y de decomps que añadieron online
    (y cómo, p. ej. con Steam / AppID 480 "Spacewar"); reconstruir en alpha la lógica del multijugador de KHCoM.
  - [x] (23/10: cabecera con versión, SO, Wine/Proton, pantalla, ajustes; cada cambio de modo; botón "Reportar un
    problema..." en la tarjeta Partida del launcher, issue prellenado con el log y la carpeta del usuario oculta)
    Registro y reportes (usuario 23/10): log más útil sin costo (cabecera con versión, plataforma, renderer,
    ajustes; cambios de modo); botón opcional "Reportar un problema" que abre un issue de GitHub prellenado (sin VPS).
    Más adelante, si se quiere automático: endpoint gratuito (Cloudflare Workers), siempre con consentimiento.
  - Todo esto antes de publicar la v2.0 (usuario 23/10 ~05:00, se fue a dormir: seguir sin pedir decisiones).
  - [x] (CEnemyGauge, capas de 560 del juego) Barra de vida de los enemigos como Re:CoM (CEnemyGauge decompilado, con su lógica como la de Sora).
  - [ ] **Bug:** un cuadro negro en medio del primer combate de Agrabah con HUD HD (el usuario dejará savestate);
    el HD debe ir igual de fluido que sin HD. Descartado en PC (23/10): combate de Agrabah con cartas/HUD/texto HD, 2451 cuadros normales y 817 con salto
    forzado, ninguna caída aislada de brillo; los savestates del usuario son del evento 000 (slot1) y de editar
    baraja (slot2), no del combate. Falta un savestate del combate o probar con Direct3D 11 real.
  - [x] Sonido al moverse en la elección HD/GBA del primer arranque (como en Opciones).
  - [x] Ajustes: "Intros HD" (apertura, créditos, finales; encendido por defecto salvo modo GBA Original),
    "Escenas HD", "HUD HD" (logo del título + cartas + HUD de combate en un solo interruptor).
  - [x] Nueva partida: preguntar "Escenas HD" y "HUD HD" (con una línea que explique qué es el HUD).
- **23/10 ~12:20: batería events jp se cortó a los 5 casos por recompilar build/pc-jp a mitad (copia bloqueada):
  case_bin ya no se corta si el build se reenlaza (sigue con su copia); debug_kit ya no se cae al imprimir japonés.
  Relanzada.**
- **23/10 ~12:00: #47 confirmó en la pre2 las pistas de Leon y todos los SFX con música HD → #44 cerrado con esa
  confirmación (mismo síntoma). Lección: un arreglo confirmado por un reportero sirve para cerrar issues gemelos
  de otros reporteros, siempre dejando la puerta abierta a reabrir. Esperan: #42, #45, #46 (#40 sin datos).**
- **23/10 ~11:20: #43 (Switch, cartas con frame skip) confirmado arreglado por el reportero con la pre1: cerrado.**
  #47 cerrado (lo cerró el reportero). Esperan respuesta: #42, #44, #45, #46 (este usaba la 1.9.14).
- **23/10 ~10:30: v2.0-pre2 publicada (solo Windows)**: #47 (pistas de Leon), launcher (calidad HD, Steam, instalar
  en carpeta, reportar), cabecera del log. Avisado en #47.
- **23/10 ~09:40: v2.0-pre1 publicada (prerelease: Windows, Switch, Android)**, avisada en #42 #43 #44 #45 #46;
  esperar si reportan mejora antes de la v2.0. APK probado en el emulador (arranca, 40 s, sin crash).
- **Vita (reporte dentro de #44):** "bastante lenta"; sin undub aunque estén las ROM US y JP en la carpeta (23/10 arreglado:
  el undub buscaba solo roms/B8CJ.gba y cargaba la ROM JP entera en 32 MB; ahora la busca por cabecera en cualquier
  .gba de la carpeta de datos y de roms/ y la lee por partes; probado en PC con un nombre cualquiera, falta la Vita); texto HD
  no seleccionable. Ninguna versión de Vita reportada a velocidad completa.
- **Switch rendimiento (#35, deko3d):** el host ya presenta por GPU; lo caro es la PPU en CPU y en un solo núcleo
  (PORT_PPU_THREADS es solo de escritorio; el invitado no tiene hilos ni TLS). Opciones: hilos en el invitado (host
  libnx + shim) o la PPU en shaders (deko3d/SDL GPU): proyecto grande, sin hardware para medir.

## v2.0 — próxima estable (pedido del usuario 22/10: "stable nueva con 2.0 y anunciar la inclusión del HD")

Es un lanzamiento grande: no publicar hasta pasar TODA la lista de verificación de abajo. PC (Windows, Linux, Steam Deck)
estable; Android, Switch, Vita, PS4, PS5, Anbernic, web como `-beta`. Al final: revisar issues, arreglar sus bugs,
publicar y anunciar el mod HD en las notas. El usuario prueba con `Probar ReKHCoM v1.9.15 completo.bat` (raíz).

Hecho desde la v1.9.14 (en main): efectos que se silenciaban con la música HD (canales del BGM por su `trackCount`),
música HD al volumen del cartucho, carga en hilo aparte, pausa en el lugar (sin tirar la cola), sigue sonando bajo
Opciones, silencio mientras carga; pausa de escena en videos HD (START/Esc; A ya no corta); 97/100 escenas HD
asignadas (`match_hd_scenes.py`); opciones HD en Mejoras de Re:CoM y en el asistente de partida nueva; inicio de
combate sin el cuadro del corazón chico; arte del título difuminado; videos HD con el renderer por software; barras de
vida sin división por cero (#40); salto de cuadros (encendido en todo); Switch #36; Vita en GPU; Discord en Linux.

### Estado 22/10 ~03:00 (traspaso: al Claude anterior se le acabaron los tokens)
- En main: todo lo de arriba + escenas HD 97/100 (`kHdSceneExtras`: 001/614/631 solo en modo Re:CoM), fundidos de
  0,3 s en videos HD (PC y Android), guardas de división (#40 y parientes), flujo de audio HD arreglado.
- Método de depuración y herramientas: **DEBUG_WORKFLOW.md**. Seguirlo.
- Falta: (1) ~~versión v2.0~~ hecho 22/10 (launcher, rc, web, CHANGELOG; tag **v2.0**: el actualizador de la PS5 lee
  "2.0" como 2.0.0 y `build_all.py`/`build_android.py` ya aceptan vX.Y); (2) builds de todas las plataformas desde el commit final;
  (3) la lista de verificación de abajo completa; (4) revisar issues (#40 abierto: arreglado, avisar al publicar);
  (5) publicar v2.0 anunciando el mod HD; **no se borra ningún release** (Quiver, 24/10).
- Copia del usuario: `Probar ReKHCoM v1.9.15 completo.bat` (actualizarla con cada build de Windows).

### Estado 22/10 mañana (Claude, sesión principal; el usuario durmiendo, /goal: pendientes + v2.0)
Hecho y en main (todo con la opción apagada = imagen idéntica a antes):
- **Cartas HD** (`hd_cards`): 97 ilustraciones, marcos rojo/verde/azul/negro y dígitos de Re:CoM; mano, enemigo, giro,
  jefes. `port_hdsprites.c` + `PortOamSetHdPart` (engine.c) + receta `port/data/hd_sprites.tsv`.
- **HUD de combate HD** (con `hd_cards`): retratos de Sora y Riku; CORTANAIPE/BLOQUEO/RECUPERA/¡NIVEL↑! en los 5
  idiomas EU (Re:CoM trae la misma traducción que el cartucho). JP no (texto japonés).
- **Título HD** (`hd_title`): logo de Re:CoM, la cinta "Chain of Memories" sin "Re:" (pedido del usuario).
- Importador: `khhd_sprites.py` y el launcher (`--hd-import <juego> --sprites-only`), idénticos; Android copia
  `hd/sprites`. Catálogo de sprites del juego: `KHCOM_HDSPRITE_EXPORT` + `hdsprite_catalog.py`.
- **Modos gráficos**: al primer arranque (graphics_mode=0) el título pregunta Modo HD / Modo GBA → GBA mejorado /
  GBA clásico; launcher "Modo gráfico" (Escoger/HD/GBA mejorado/GBA clásico); Opciones "Restablecer según el modo"
  (confirmar dos veces), filas Cartas HD y Título HD.
- **GBA clásico**: vista Original con borde estilo Super Game Boy (`gba_border` 1..3, 384x216, x5 a 1080p). Las 3
  propuestas: `pruebas/Bordes GBA clasico (elegir 1, 2 o 3).png` → **espera que el usuario elija** (por defecto 1).
  Partida nueva en clásico ofrece "Vista amplia" (el fondo muestra el ancho).
- Bug arreglado: el renderer por software de SDL colgaba con quads alineados espejados/girados/fuera del lienzo.
**Espera al usuario:** Darkside (sesión darkside-arte, commit f14676420 en claude/arenas, sin merge; copia de prueba
en pruebas) y elegir borde 1/2/3.
**Sigue (Claude):** depurar HD en todos los idiomas y plataformas (Android 64, consolas por el puente), intro del
título (logo como fondo afín, sigue GBA), revisar issues, lista de verificación y builds v2.0 (con OK de Darkside).

### TRASPASO 22/10 ~06:30 (al Claude principal se le acaban los tokens; el usuario se fue a dormir)
- Orden: (1) Cartas HD: seguir HUD_HD_RECOM.md "Traspaso" pasos 1-7; (2) cuando Darkside esté aprobado (sesión
  arenas-jefes, [local_d3b029d7]) y Cartas HD en beta: pedir OK al usuario y hacer los builds de la v2.0 con la lista
  de verificación de abajo; (3) modos GBA/HD y lo demás de esta sección; (4) datos de Re:CoM para el modo Re:CoM.
- Repetición sola del combate Android US 124 quedó esperando a que el emulador se libere: `build/android-124-alone.log`.
- Baterías de escritorio restantes (battles/riku/monkey/skip) siguen en `run_batteries.py`; `build/pc-*` ya es HEAD.

### Estado 22/10 ~05:30 (Claude, sesión principal)
- Hecho y en main: puntos 1-4 de abajo, arte x1 + Agrabah encendida, versión v2.0, suelo isométrico rápido, presentación
  sin copia. Comprobado en el build v2.0: combate EU/US/JP limpio, `test_hd_audio.py` 0/9, `match_hd_scenes --check`.
- **Espera al usuario:** (a) su OK para gastar recursos en los builds de la v2.0 (lo pidió el 22/10: "preguntarme antes
  de empezar a usar recursos en la 2.0"); (b) aprobar las arenas de jefes que entrega la sesión "arenas-jefes"
  (Darkside primero; 7 de 8 pendientes); (c) permiso para borrar scratchpads viejos de %TEMP%\claude (~5,4 GB).
- En curso: repetir solo el combate Android US 124 (timeout con dos corridas en el emulador): `build/android-124-alone.log`.
- Después de la v2.0: las 160 filas de la imagen GBA en varios núcleos (RENDIMIENTO.md) para 60 fps a x1.
- Disco: C: ~5 GB libres por `pagefile.sys` 38,7 GB (falta de RAM: emulador + VM + baterías); se achica al reiniciar.

### Nuevo 22/10 (usuario): HUD HD y cartas HD desde el Re:CoM del 1.5+2.5 del jugador
- La v2.0 sale cuando el arte de Darkside esté aprobado (sesión arenas-jefes); mientras, esto.
- Extraer de la instalación del jugador (D:/SteamLibrary/steamapps/common/KINGDOM HEARTS -HD 1.5+2.5 ReMIX-) los
  gráficos del HUD de Re:CoM que coincidan con los de GBA, para el HUD HD (`hd_hud`, hoy piloto con piezas hechas a
  mano/IA), y las **cartas** en una opción aparte nueva: **Cartas HD**. Nada se distribuye: lo importa el launcher,
  como los videos y la música.
- Investigar documentación (OpenKh, decomp/ingeniería inversa de Re:CoM) de los formatos de textura y de la lógica.
- Inventario de texto del HUD que aún no es HD (números y letras) que pueda pasar a HD sin tocar mucho el HUD.
- Estado 22/10: texturas HD halladas (226 cartas, marcos, dígitos, fuentes); 1:1 = cartas de combate y del menú de
  barajas → **Cartas HD** primero. Diseño y estado en HUD_HD_RECOM.md. La v2.0 espera a esto como beta.
- También (usuario 22/10), si los gráficos de Re:CoM lo permiten: **pantalla de título y transiciones en HD**, y
  nuestro **menú de Opciones en HD**.
- **Primer arranque (juego) y launcher: elegir "Modo GBA" o "Modo HD"**, cada uno con su descripción. El Modo GBA
  pregunta después **"GBA mejorado"** (nuestro estándar actual: vista amplia, mejoras) o **"GBA clásico"** (como en
  un emulador: la imagen 240x160 al mayor entero que entre en la ventana, y el resto con un **borde decorativo
  propio estilo Super Game Boy**; el borde es arte nuevo: se entrega con antes/después para aprobar). Se puede
  cambiar luego en el launcher y en Opciones.
  Modo HD = todos los assets HD y todas las mejoras encendidas. GBA mejorado = las mejoras del port como hoy, con
  los extras HD (texto HD, etc.) apagados por defecto. Al empezar partida nueva se sigue preguntando y la elección
  se guarda por partida (como el asistente de partida nueva actual).
  **GBA clásico (usuario 22/10): todo apagado (vista amplia, texto HD, QoL): casi un emulador, pero nativo.** Aun
  así, al empezar partida nueva pregunta si activar lo que aplique (QoL, vista amplia); al mover el interruptor de
  la vista amplia el fondo de esa pantalla cambia a vista amplia para que se note el cambio.
  Decisiones por defecto mientras el usuario no diga otra cosa: Modo HD sin importación = ofrece importar primero;
  borde estilo Super Game Boy = 3 propuestas para elegir; scratchpads de otras sesiones en %TEMP% = no tocar.
- **Usuario 22/10 ~05:40:** en el launcher (Settings) el modo gráfico viene por defecto en **"Escoger"**: así el
  juego muestra la elección de modo la primera vez que el jugador empieza. En nuestras Opciones, junto a "volver todo
  a default", una opción para **volver a los valores por defecto de un modo** (HD, GBA mejorado, GBA clásico), por
  si alguien cambia de opinión.
- **Usuario 22/10:** rescatar **todos** los assets HD de Re:CoM que se puedan (rastrear todo el .pkg, reconocer lo
  que es 1:1 como las cartas) y lograr lo máximo posible: **pantalla de título HD** con esos assets y nuestras
  opciones, HUD, menús. Inventario en HUD_HD_RECOM.md.
- Darkside: sesión [darkside-arte] (worktree D:/01Claude/arenas-wt) lo termina y lo entrega para aprobar.
- Después (pedido del usuario): datos duros de Re:CoM (dificultades, valores de cartas/enemigos, cambios respecto
  a GBA) para el modo Re:CoM, en lógica y jugabilidad, con pruebas para no meter bugs.

### Bugs encontrados y cerrados el 22/10 (todos en main)
- Escena HD: destello de la imagen del evento al terminar (53f756bc7).
- Música del campo bajo el video HD y corte seco; salto sin fundido de imagen y sonido (53f756bc7, PC y Android).
- Pausa del video HD muda (1c8a17352).
- Agrabah 107/108/109 aprobadas con honores pero apagadas para jugadores (d94ce3b56).
- Suelo isométrico: tabla única recalculada por píxel, <5 fps a x1 en 004/057/061/074 (también pesaba en Android,
  web y consolas, un núcleo): 31 ms por cuadro, píxel a píxel igual (e1b635d65).
- Batería: resultados del 21/10 viejos (Event_0, Pooh) ya arreglados; 0 sitios abiertos.
- Pendiente de confirmar: timeout Android US combate 124 (Agrabah) con dos corridas en el mismo emulador; repetir solo.

### Pendientes v2.0 (en orden)
- ~~Arte extendido a x1 en 1080p~~ **hecho 22/10**: a x1 1920x1080 todo lo aprobado ya llega al borde (Guard
  Armor, 004, 008, 057, 061, 074, Agrabah 107/108/109; `build/x1art`). Agrabah 107/108 y 109 (con honores, 21/10)
  estaban detrás de `KHCOM_AGRABAH_WIDE`/`KHCOM_AGRABAH109_WIDE` esperando la integración: ahora encendidas para los
  jugadores (la variable en 0 las apaga para antes/después); `test_agrabah_backdrop.py` 11/11,
  `test_agrabah109_backdrop.py` 10/10 (EU/US/JP, rectángulo GBA idéntico).

1. ~~Escenas HD: disparo real~~ **hecho 22/10**: `test_hd_scenes.py` (cada evento desde el menú de depuración,
   video cortado a 1 s): EU 97/97 en modo original y 97/97 en Re:CoM; 001/614/631 solo en Re:CoM y en su orden. Todos
   los eventos (también los mudos) pasan por `UpdateEventSeq`.
2. ~~Transiciones~~ **hecho 22/10**: al terminar la escena HD el fundido del evento empieza ya en negro (antes volvía
   la imagen del evento a brillo 234 y se fundía 64 cuadros); saltar el video funde imagen y sonido 0,3 s (PC y
   Android); el sonido del juego y la música HD bajan en 150 ms antes del video (antes 200 ms de la música del campo
   sonaban debajo y se cortaban). Medido con el driver "disk" (`build/hdtrans`).
3. ~~358/2 Days~~ **hecho 22/10**: KHDays FM (melonDS, github.com/vitor251093/KHDays_FM) pausa el video HD con
   Saltar/Continuar y sonidos de menú, y baja la música 1 s antes. Tomado: la pausa del video HD suena como los menús
   del juego; la música ya baja antes del video (150 ms, con la pantalla en negro).
4. ~~Consolas~~ **documentado 22/10** en HD_MOD.md (decodificador por consola, qué falta en cada host).
5. Revisar issues abiertos al final, arreglar bugs, responder breve.

### Lista de verificación antes de publicar (no saltear)
- **Sin Switch en la v2.0** (usuario 24/10): queda en la última prerelease de la 2.0; vuelve con el motor nuevo
  comprobado (SWITCH_MOTOR.md). Sacar el `.nro` de la publicación de la v2.0.
- Correr `port/tools/test_update.py` con el paquete nuevo contra `respaldos/releases-estables` y
  `--olds respaldos/releases-prueba` (las que Quiver tiene: v1.9.15-pre1 verificada, v2.0-pre2): todo ok antes de publicar.
- **No se borra NINGÚN release ni archivo** (estables ni de prueba): Quiver nos aceptó el 24/10 y sus mantenedores dijeron
  que borrar parecía sospechoso. Publicar = solo agregar. README con la versión nueva.
- Copiar los archivos de la estable publicada a `KHCoMPC/respaldos/releases-estables` (originales, mismo SHA).
  24/10: v1.9.10 y v1.9.8 republicadas desde ahí (hash verificado igual al original), Latest sigue v1.9.14.
- `release_check.py v2.0` antes y `--post` después (versión en launcher, rc, CHANGELOG, web, paquetes, Quiver).
- `check_sources.py` (lo corre build_android y release_check): constantes del shim de consolas.
- Builds: Windows, Linux, Android, PS4, Switch, Vita, PS5 (VM), web — desde el commit final; ningún paquete viejo
  (los scripts de VM ya no reusan paquetes, verificar "fetched" en los logs).
- Combate de prueba con cada ejecutable de PC (`KHCOM_BATTLE=10:4`), sin `bios:`/CRASH.
- `debug_kit.py report` (baterías de escritorio: scenes, events, wide, rooms, battles, riku, monkey, skip) sin
  sitios nuevos.
- Android 64 (`run_android_full.py` / pasos `android ...` de run_batteries): imagen movida + jefes EU/US/JP, Riku.
- `test_hd_audio.py` (ningún efecto tomado por música), `test_hd_audio_flow.py` con `uv run --with numpy --with
  soundfile` (driver "disk": 0 cortes, 0 solapamientos, 0 saltos audibles; incluye Opciones abiertas).
- `test_frameskip.py` (lógica idéntica con salto de cuadros).
- Videos HD: `KHCOM_HDMOVIE_PAUSETEST` (pausa y salto), renderer por software, Android (`KHCOM_MOVIE=1` en el emulador,
  `build/hdtest/run_hdmovie.py` en khcom-claude-hdmov).
- `match_hd_scenes.py --check` (asignaciones consistentes).
- Copia de prueba para el usuario + `.bat`; notas cortas; nunca decir en público que algo no se probó en hardware.

## 21/10 noche (usuario ausente, /goal: bugs y pendientes sin imágenes)

- **v1.9.15-pre1 publicada (prueba: Windows, Switch, Vita)**, comentada en #35 y #36. Incluye: #36 (táctil oculto en
  pausa/Opciones, Minus + Plus 3 s abre Opciones), salto de cuadros (`experimental_frame_skip`, encendido en todo tras
  `test_frameskip.py` 24/24 idénticos y 1/3600 saltados en PC con VSync; RENDIMIENTO.md), Vita en GPU (SDL2 GXM de
  vdpm), cola de audio del puente en unidades de entrada.
- **#36 cerrado** (DrunkECheese probó la pre1: sin quejas).
- **No se pueden cerrar desde acá (21/10 noche):**
  - D. Videos HD en Linux: el paquete es i386 con sus propias librerías Debian; haría falta empaquetar ffmpeg i386
    (libavcodec/libavformat + dependencias, decenas de MB) o portar openh264 + minimp4 + un decodificador AAC.
    Decisión de tamaño/diseño para el usuario. Consolas (sceAvPlayer, nvdec): sin hardware para verificar.
  - I'. PS4 a la GPU (Piglet): sin PS4 ni emulador para verificar; el rasterizador por software sigue, y el salto de
    cuadros ya mantiene la velocidad si no llega.
  - Quitar los worktrees viejos. Los releases se conservan todos (Quiver, 24/10).
- **C. Videos HD en Android: hecho** (RGB por defecto, verificado en el emulador; HD_MOD.md). Rama en main.
- **F. Discord en Linux: hecho.** VM Debian con un socket falso: handshake y SET_ACTIVITY (mundo, piso, nivel, combate).
- #34 cerrado (confirmado). #39 (recortar relleno) respondido: idea, no bug.
- Builds de consolas: `build_ps5.py` enlaza con archivo de respuesta (límite de 32 K) y lee bien los `.d` con rutas de
  Windows; Switch/Vita/PS5 ya no reusan un paquete viejo si la VM falla; Vita rellena 8 KiB si `vita-elf-create` no
  tiene lugar.
- Baterías: `run_batteries.py` suma battles, riku, monkey y skip (EU/US/JP); el AVD se encuentra con ANDROID_AVD_HOME.

## Carpetas de KHCoMPC (21/10, pedido del usuario)

La raíz quedó en: `jugar/` (copias ReKHCoM-vX con partidas del usuario + su .bat), `pruebas/` (copias que esperan
aprobación + .bat), `roms/`, `arte/` (fuentes, imágenes, Showcase, Trailer), `respaldos/`, `toolchain/` y los repos.
Cada una tiene `LEEME.md`. **Codex:** `package_preliminar_v1.py` ahora escribe en `../pruebas/ReKHCoM-preliminarV1`,
`crystal_parity.py` lee `../respaldos/` y el tráiler sale en `../arte/Trailer/` (main); trae main antes de usarlos.

## Reparto

| Quién | Pendiente |
|---|---|
| **Claude** | **J + I a main**: cuando las dos baterías de arriba terminen limpias, build pc-eu/us/jp, prueba de combate, merge a main, APK de prueba. Con el OK del usuario: prerelease y pedir a #22/#34 que prueben; #37/#38 se cierran con la confirmación de xsm2 |
| **Claude** | **A. Batería Android 64 completa**: EU jefes con Riku, US/JP menús y jefes; luego otros perfiles (Android 13/15, tablet, plegable como el Mate XT) y el bot de partida (`ANDROID_64.md`) |
| **Claude** | **B. Batería de escritorio** (`run_batteries.py`): cada `bios:` nuevo se arregla (`KIT_DEPURACION.md`) |
| **Claude** | **C. Videos HD en Android**: rama `claude/android-hd-movies`; el emulador se cuelga en la primera textura YUV. Probar `KHCOM_HDMOVIE_RGB=1` (b4cb4730d) en el emulador; a main solo verificado (`HD_MOD.md`). Luego **D**: consolas y MP4 en Linux |
| **Claude** | **E'. Reportes nuevos (21/10 ~16:15, prioridad del usuario)**: #22 xsm2 Poco F8 Ultra y #34 Mate XT con la pre2: "The game image could not be loaded" = bug J (imagen de 64 bits sin hueco en 0x01000000), arreglado en `claude/perf-android-reloc`; xsm2 no llega a Settings para el log (la rama pone "Share log" en la caja de error). Odin 2 Portal anda bien con la pre2. Casos en la batería: grupo `issues` de `test_android_scenarios.py` (#33 evento 099, Bosque 100 Acres) y paso `android eu move` con `KHCOM_IMAGE_MOVE=1`. `mode_vsbattle` sale de la batería: arnés sin compañero de link (HARNESS_ONLY) |
| **Claude** | **E. Issues**: #33 (Trickmaster, arreglado en pre2) y #35 (Switch lento: si sigue con la GPU, deko3d) esperan a los testers; #36 (Switch se cierra en el título): comentario listo en `build/issue36-comment.md`, sale con la próxima prueba; #25 se cierra cuando el mod HD salga en una estable |
| **Claude** | **G. Integración de Codex**: traer sus commits (`khcom-codex` codex/preliminar-v1 y `khcom`) a main con build y prueba (al día hasta 457d10854) |
| **Claude** | **I'. Rendimiento en consolas**: Vita a la GPU (necesita OK del usuario para `vdpm sdl2` en la VM), PS4 a la GPU (Piglet) (`RENDIMIENTO.md`) |
| **Claude** | **H. Próxima estable**: solo con el OK del usuario; `release_check.py` antes/después, todas las plataformas, web |
| **ChatGPT/Codex** | 39: rutas completas de Sora y de Reverse/Rebirth |
| **ChatGPT/Codex** | 16–24: arte de jefes y escenas, de a uno con antes/después, con el método de abajo ("Para Codex: cómo hacer el arte") |
| **Usuario** | Probar `Probar Discord.bat` y `Probar ReKHCoM v1.9.13 pre2.bat` (pantalla completa sin parpadeo, mod HD, volver al título) |
| **Usuario** | Aprobar el launcher nuevo (`Probar launcher nuevo.bat`, commit 07a6fa9ab); revisar 61–62 cuando Claude las muestre |

## Para Codex: cómo hacer el arte (16–24), según lo aprobado (21/10/2026)

Pedido del usuario: seguir el mismo método con que se aprobaron los arreglos visuales.

- **Procedimiento:** `WORKFLOW_ARTE_AMPLIO.md` (paso a paso), reglas en `LOGICA_APROBADOS.md`
  (mandan ante la duda), entrega en `PRUEBAS_VISUALES.md`, técnica de combates en
  `BATTLE_SCENERY_TECHNIQUE.md`.
- **Modelo "aprobado con honores" (20/10): Guard Armor, escena 008.** El patio del combate cubre
  también el evento previo: la extensión se elige por los cuatro mapas del fondo, no por el modo
  (`port_ppu.c`, `guardPatio`). Lección: buscar TODAS las apariciones de un fondo (combate, evento
  previo, derrota, revancha) e identificarlo por sus datos, nunca por modo/número de combate/bandera.
- **Arte:** solo tiles del juego (repetir/voltear); bordes cortados se cierran con el borde propio
  volteado; nunca ornamentos repetidos o cortados a la mitad (relleno de pared lisa); pixel art
  nuevo solo donde falte, con píxeles y paleta del juego. El rectángulo GBA queda idéntico.
- **Pruebas:** zoom x3 entero (`zoom=6`) a 1920x1080, 2560x1080, 3440x1440 y 3840x1080
  (`KHCOM_WINDOW`), antes/después.
- **Entrega:** copia jugable + `.bat`, de a un jefe/escena, y esperar la aprobación del usuario;
  lo aprobado se documenta en `LOGICA_APROBADOS.md`.

## Vita y Switch (20/10, 17:00)

Por qué es distinto de PC/PS5/PS4: el juego lee los datos del volcado del usuario, y esos datos
llevan punteros absolutos de GBA (0x08xxxxxx). PC, Android, web, PS5 y PS4 cargan la ROM en
0x08000000 y los punteros valen tal cual. Switch (AArch64, código desde 0x08000000, homebrew sin
procesos de 32 bits) y Vita (ARM32, usuario desde 0x81000000) no pueden usar esas direcciones.

Nuestro método (PORT_GBA_RELOC, `include/gba/gba_space.h`): todas las regiones GBA son arreglos del
programa; al cargar la ROM se corrigen sus punteros con la lista exacta del enlazador
(`port/data/rom_pointers_*.inc`, de reenlazar la ROM del decomp con `--emit-relocs`), y los
accesos sueltos a direcciones de bus se traducen (`GbaSpacePtr`). Una misma imagen sirve a las dos.
ggdrn/KH_COM_VITA (MIT) hace lo equivalente de otra forma: enlaza los datos dentro del ejecutable
(el enlazador de Vita corrige los punteros), los pone a cero y los rellena desde la ROM del usuario
al arrancar; los punteros dentro de bloques binarios los traduce con `GBA_PTR()` en cada uso.
Las dos vías necesitan que el decomp compile la ROM idéntica al cartucho.

Hecho:
- Modo reubicable: cabeceras, símbolos fijos (`rom_symbols_reloc.s`), build de comprobación en
  Windows (`configure_pc.py --reloc`) con el rango del bus reservado sin acceso.
- Build idéntica de GBA recuperada (7c7fb0f09, 62777ed3f): ROM EU/US/JP iguales al cartucho.
- Listas de punteros de EU/US/JP (`gen_rom_pointers.py`, 132037/98603/98035 palabras), cada una
  comprobada contra el cartucho; incluyen las desalineadas (GOTO/PATT de las canciones).
- Switch (`port/switch`, README): imagen AArch64 de punteros de 32 bits para Cortex-A57 (sin
  `-mcpu` clang emitía `xpaci` de ARMv8.3 y caía), memcpy/memset de 16 bytes, `.nro` de las tres
  regiones (`build_switch.py host`, devkitA64 de la imagen Docker devkitpro/devkita64 porque
  apt.devkitpro.org responde 403).
- Vita (`port/vita`, README): misma imagen reubicable en ARMv7 (clang, Cortex-A9), aislada en un
  objeto con solo `guest_main`/`gHostTable` globales y enlazada con el host compartido
  (`switch_host.c` con `__vita__`: rasterizador a 960x544, sceAudioOut, sceCtrl); un ejecutable por
  región (eboot.bin EU, eboot_us/jp.self). VitaSDK en la VM de consolas. `build_vita.py test` corre la
  misma imagen en Linux ARM de 32 bits.
- Verificado en la VM ARM64: título, menús y la batalla KHCOM_BATTLE=10:4 en Switch y en Vita; el
  fotograma 1490 sale idéntico píxel a píxel en las dos imágenes.

Sin verificar (no hay consola ni emulador aquí): mapeo de memoria de código en Switch real, pantalla,
audio y mandos del host de Vita, rendimiento. Llega como reporte de quien lo pruebe.


## Esperando a terceros (no se trabaja)

- Quiver: issue tgeorgiadis/quiver-launcher#182, pedimos levantar la retención del
  repositorio (VirusTotal 0/68; la causa fueron releases borrados mientras los revisaban).
