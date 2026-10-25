# Flujo de depuración (22/10/2026) — para quien siga (Claude u otro modelo)

Regla del usuario: cuando aparece un bug, buscar sus "parientes" (la misma clase de error en otros lugares) y
seguir solo mientras sigan apareciendo; nada de bucles de pruebas sin resultado. Cada clase encontrada deja una
prueba o un chequeo reutilizable. Lo terminado se anota en PENDIENTES_VIGENTES.md.

**Regla (usuario 22/10):** un caso de batería que se corta por tiempo, se cae o sale con código distinto de 0 **es un
bug**: se reproduce solo enseguida (escritorio con `bios:` primero, luego el arnés del dispositivo), se explica, se
arregla y deja un caso de regresión. Nunca "repetir cuando haya tiempo" (US 124/126 de Android quedaron así el 22/10 y
esa noche llegó #46). Después de cada arreglo: buscar sus parientes en el resto del juego (misma función, mismo tipo de
combate/evento más adelante en la historia, las tres versiones) y correrlos.

## Clases de bugs encontradas y cómo se buscan sus parientes
| Clase | Origen | Cómo buscar parientes / prueba |
|---|---|---|
| Acceso suelto que el PC recupera ("bios:") y cierra Android/web/ARM | Event_0, Bosque 100 Acres | `debug_kit.py report`, baterías `run_batteries.py` |
| División por un valor aún 0 (vida máxima antes de iniciarse) | #40 | `grep -rnE "/ *[a-z_>.]*(maxHp|count|Count|total)" src`; guardar con `#ifdef PLATFORM_PC` (hecho: hpply, hpoth, hpenm, bos_lst, bos_pc) |
| Rango por un máximo en vez de la cantidad real | efectos silenciados con la música HD (`MAX_MUSICPLAYER_TRACKS`) | `grep "+ MAX_"`; `KHCOM_AUDIOCHECK=1` + `test_hd_audio.py` |
| Un cuadro de desfase entre un efecto del port y un fundido del juego | corazón al iniciar combate (`btl_start.c` `timer <= 43`) | capturas de la imagen presentada `KHCOM_PRESENTSHOTS=a-b`, `KHCOM_REGLOG=a-b` |
| IDs de evento del menú de depuración con 0x8000 | Event_0, escenas HD | buscar `eventId ==` en port/src y enmascarar `& 0x7FFF` |
| Constantes que el shim de consolas no tiene | SDL_EVENT_MOUSE_BUTTON_UP | `check_sources.py` (corre en build_android y release_check) |
| Scripts de build que reusan paquetes viejos / línea de 32 K | Switch/Vita/PS5, PS4 | ver "fetched" en logs; rmtree de la salida antes de la VM |
| Audio HD: pausa que tira la cola, solapamiento con la música del cartucho | menús/Opciones | `test_hd_audio_flow.py` (driver "disk", tiempo real) |
| Fundido del juego que arranca desde la imagen vieja tras algo del port (video HD) | escena HD: el evento volvía a brillo 234 y se fundía | `KHCOM_PRESENTSHOTS` + brillo medio por cuadro (`test_hd_transition_audio.py`) |
| Cola de audio que sigue sonando cuando el juego se congela (video, menú) | 200 ms de música del campo bajo el video y corte seco | driver "disk": volumen cada 50 ms en el borde (`test_hd_transition_audio.py`) |
| Caché de una sola entrada compartida entre capas/llamadas: se recalcula en cada uso | suelo isométrico (`sIso`), <5 fps a x1 | `perf_x1.py` (un caso que no termina = sospechoso); buscar `static struct` con clave de puntero; `compare_ppu_builds.py` para probar que no cambia un píxel |
| Trabajo por píxel que solo depende del tile | `IsoEntry` caminaba la diagonal en cada píxel | memo por tile de la coordenada (no del valor: el mapa se lee en vivo) |
| Arte aprobado detrás de una variable de prueba | Agrabah 107/109 nunca encendidas para jugadores | `grep 'getenv("KHCOM_' port_ppu.c`: cada variable de arte aprobado debe ser "0 apaga", no "1 enciende" |
| Quad alineado a los ejes en el renderer por software de SDL (espejado, girado 90°/180° o en parte fuera del lienzo): SDL lo convierte en copia escalada y lee fuera de la textura (cuelga) | Cartas HD: carta enemiga girando (`port_hdsprites.c`, `Quad`) | correr con `SDL_VIDEO_DRIVER=dummy` (renderer por software) combates con cartas que giran (`KHCOM_BATTLE=10:4:0`, A cada 200 cuadros); pariente a revisar: `PortHdHudDrawSprites` usa `SDL_RenderTextureRotated` con flip |
| Triángulo muy grande en el renderer por software de SDL: no dibuja nada, sin error (logo del título en la intro, 1700 px) | `port_hdsprites.c`, `Quad` divide en una grilla | volcar la imagen justo después de dibujar (`SDL_RenderReadPixels`) con `SDL_VIDEO_DRIVER=dummy`; pariente: cualquier `SDL_RenderGeometry` de quads grandes |
| Función de SDL nueva en `port/src` que el shim de consolas no tiene, o variable del camino con biblioteca estándar | `SDL_SetAudioStreamGain`, `sStream` (fundido antes del video HD) | `check_sources.py` compila `port/src` contra el shim (10 s) |
| Algo que el juego dibuja en la capa de pantalla (`sScreenBg`) y no en la imagen | BG2 del logo en la intro del título | registrar `sOverlay`/`sScreenBg[b]`; `KHCOM_DBGSIDE` dice por qué rama presenta |
| Cuadro "sostenido" que el bucle del grupo da vuelta (el largo entero = último cuadro = vuelve a 0) | medidor de vida al 100 % a media rosca | todo `PortHdLayoutDraw` con un valor del juego como cuadro va con `HD_LAYOUT_HOLD`; `test_hd_menus.py --only gauge` (vida 3..100 %) |
| Sprite de UV de ancho/alto cero tratado como vacío: es una columna estirada, o (UV un punto) una **máscara** que recorta lo que sigue | barra del medidor invisible, anillo a media rosca con poca vida | gemelo Python `seqdraw.py`; buscar en cada .lad sprites con u0==u1 o v0==v1 antes de usarlo |
| Layout de PS2 (UV×2) con la textura remasterizada: el remaster movió piezas | cuartos del anillo y barra | usar siempre la entrada remasterizada `UK_*.lad` del .SPR; comparar UV base×2 contra la remasterizada |
| El menú pasa el sprite del título a una copia en un BG al quedar quieto (a veces liberando sus tiles) | mapa, diario, nivel, elegir mundo, tienda, barajas | `test_hd_menus.py`: el título HD debe seguir en el cuadro quieto (registro `KHCOM_HDDRAWLOG`); `grep -n "BgHeaderMap\|LoadBgMap" src/menu src/card` junto a cada título |
| La copia en BG tiene más prioridad que el sprite: la capa "lo que tapa" vuelve a poner los píxeles GBA encima del HD | elegir mundo, tienda | prioridad del sprite que se mantiene ≥ la del BG (`SetBgPriority`); misma prueba |
| Tiles compartidos entre idiomas: el juego carga otro conjunto que el que nombra el frame | tienda (`gMsTopBarTiles` + frame en español) | `check_hd_sprites.py` compara las filas con los `LoadObjTiles` del código; registro `hdfind` de mismatches |
| Dos filas HD con el mismo nombre: la segunda pisa la imagen de la primera | DIARIO / JOURNAL | `check_hd_sprites.py` (nombres únicos) |
| Palabra partida a mitad de línea: arrastra las esquinas itálicas de la vecina | WELTEN-KARTEN | armar letra por letra con `sblit`; mirar la hoja de carteles a 1:1 |
| Constante o función de SDL que el shim no tiene (`SDL_clamp`, `SDL_BLENDMODE_ADD`, `SDL_GetRenderClipRect`) | medidor, piezas fijas | `check_sources.py` **antes** de cada commit (con `&&`, no `;`) |
| `getenv` en un camino por píxel, por fila, por sprite o de audio: en Windows bloquea y recorre el entorno (Jafar 32:9: un cuadro por segundo, parecía un cuelgue) | interruptor de prueba de Jafar (24/10); antes por fila en `DrawObjs`, patio de Guard Armor, bandas del título; por píxel en el HUD HD | `PORT_ENV_ONCE(var, getenv\|SDL_getenv, "KHCOM_...")` (port.h) o leerlo al iniciar el cuadro; buscar `getenv(` dentro de funciones de dibujo/audio. Una corrida que "se cuelga" sin crash: medir avance (perro guardián temporal que registra fila/columna y ritmo) antes de buscar un bucle |
| Búfer de mapa del juego que en PC es un global nativo (`EWRAM_COMMON` fuera del espacio GBA): `PORT_GBA_MAP_MEMORY` lo rechaza y la capa pierde la vista ancha | sala y genio de Jafar (`gBosJfMapBuffer`, `gBosJfMajinMapBuffer`) | `grep -rn "EWRAM_COMMON" src \| grep -i map`, ver qué `SetBgMapBlocks` apunta a uno; registro `latch` de `KHCOM_PPULOG` (0 = capa no reconocida) |
| El mapa envuelve en vertical pero la vista ancha solo en horizontal: franja negra donde la sala sigue arriba del mapa | Jafar filas 512..543 (las filas 0..31 del mapa) | dibujar esas filas en la extensión del arena; buscar salas cuyo dibujo cruce y = 512 |
| Entrada táctil que en un menú manda dos direcciones a la vez (diagonal de la cruceta: abajo + derecha) | Android 24/10: al bajar en Opciones cambiaba la resolución y se abría Logros solo | `TouchKeys(1)` en `touch_android.c` (solo el eje dominante en menús); prueba `test_android_touch.py` (caso diagonal); pariente: todo lo que reenvíe bits GBA como teclas a un menú |
| "Fallo" de batería que no es del juego: código de salida 0x40010004 (1073807364, proceso terminado desde fuera) o 0xC000026B (3221226091, Windows cerrando sesión/reiniciando) en un bloque seguido | events-us 23/10: 37 eventos "BAD" = reinicio del PC; riku-eu 153/154 = un `taskkill` | mirar el código en el .txt de la suite; repetir esos casos solos (`--only`, `--work` aparte) antes de buscar un bug; nunca `taskkill /IM khcom.exe` con baterías corriendo |
| Escalado LINEAR en el renderer por software de SDL al pasar a pantalla completa: lee fuera de la textura (0xC0000005 en SDL3.dll, `SDL_RenderPresent`) | filtro "Suave" + pantalla completa (`test_text_options.py`, existía antes del 24/10) | `SmoothScaleMode()` (port_main.c) usa PIXELART con el renderer "software"; el HUD HD ya lo hacía (NEAREST); buscar `SDL_SCALEMODE_LINEAR` en texturas que se estiran a la ventana; reproducir con `SDL_VIDEO_DRIVER=offscreen` |
| Símbolo de SDL en código que también compilan los invitados sin SDL completo (consolas, Android 64): `SDL_max/SDL_min` en port_ppu.c, `SDLK_TAB` en el menú de logros (25/10), `SDL_PushEvent` en «Salir del juego» (25/10: `check_sources` no lo vio; compilar `build_android64.py --regions eu`) | lista Oficiales/Re:CoM, ventana de sprites de Jafar | `check_sources.py` (PYTHONIOENCODING=utf-8) antes de subir; teclas nuevas: agregarlas a `port/ps5/guest/include/SDL3/SDL.h` (lo comparten PS5 y Android 64); en port_ppu.c usar ternarios |
| Rendimiento medido sin presentar (`SDL_VIDEO_DRIVER=offscreen`): no cuenta subir cada cuadro a la ventana | #140 Steam Deck a 25 fps con 6 ms por cuadro en la VM | medir con una ventana real (Xvfb en la VM Linux, `vmshare-gpu/gpu2.sh`) y mirar `Renderer:` en kh_com.log; software = sospechoso |
| Paquete Linux con su propia libc: el driver de GPU del sistema (Mesa más nuevo) no carga y SDL dibuja por software, sin error | #140 (REKHCOM_GPU=1 sumaba carpetas pero seguía con la libc del paquete) | `ReKHCoM.sh` arranca con el cargador de 32 bits del sistema cuando hay Mesa de 32 bits; orden: `lib/sys` (SDL3), carpetas del sistema, `lib/` al final (SDL abre X11/audio con dlopen y nombres sin versión: el RUNPATH del juego no los alcanza). Probado en la VM Debian 12 (Xvfb + Mesa i386); la imagen nocloud de Debian 13 se queda en el asistente de primer arranque (no lee la consola serie) |
| Batería que «falla» toda junta en segundos: el build no enlazó (build.ninja viejo sin los archivos nuevos) | 25/10 15:22: desktop build exit 1 (`PortHdGauge` sin definir) y cada suite EU exit 1 | `debug_kit.py --build` reconfigura siempre (`configure_pc.py`); mirar `battery-desktop_build.log` antes de leer las suites |
| Arreglo publicado como «hecho» sin probarlo en el camino real del jugador | #140: pre18 dijo «prueba REKHCOM_GPU=1» sin haber visto nunca `Renderer: opengl` | antes de responder un issue, la prueba debe mostrar el síntoma resuelto (aquí: `Renderer:` y fps con ventana); lo que no se vio, no se promete |

## Herramientas (todas en port/tools)
- `debug_kit.py run/report`, `run_batteries.py --detach --status` (escritorio + Android; nunca matar baterías desde la sesión).
- `test_hd_audio.py` (efectos vs música), `uv run --with numpy --with soundfile python test_hd_audio_flow.py --hd <hd>` (cortes, solapamientos, saltos audibles; incluye Opciones abiertas `KHCOM_OPTIONS_AT`+`KHCOM_OPTIONS_HOLD`).
- `test_frameskip.py` (lógica idéntica con salto de cuadros), `match_hd_scenes.py --check` (escenas HD por diálogo en 5 idiomas).
- Escenas HD: `test_hd_scenes.py --hd <hd> [--recom] [--only EVENT_...]` (cada evento pide su escena y abre su video;
  `KHCOM_HDMOVIE_SECONDS=<n>` salta cada video a los n s, con su fundido).
- Sonido alrededor de una escena HD: `uv run --with numpy python test_hd_transition_audio.py --hd <hd> [--pause 2]`
  (driver "disk": volumen cada 50 ms y saltos de muestra; imágenes presentadas para los fundidos).
- Videos HD: `KHCOM_MOVIE=1`, `KHCOM_HDMOVIE_PAUSETEST=<s>`, `KHCOM_HDMOVIE_SHOT=<s>`, `KHCOM_RECOM=1`, log "hd scene: event N".
- Ganchos: `KHCOM_SKIPTEST`, `KHCOM_WIZARD_OPEN`+`KHCOM_WIZARD_KEYS`, `KHCOM_VPAD=1` (cuenta cuadros de menú), `KHCOM_FRAMELOG` (+columna fit), `KHCOM_HDAUDIOLOG`.
- Android: `build/run_android_full.py` (emulador, apagarlo al terminar), `KHCOM_IMAGE_MOVE=1` predijo bien el Poco F8 Ultra (#22).
