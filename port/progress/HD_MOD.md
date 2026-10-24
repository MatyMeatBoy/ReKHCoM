# Mod HD: videos y música de KINGDOM HEARTS -HD 1.5+2.5 ReMIX- (21/10/2026, para v1.9.21)

Pedido del usuario: usar el Re:Chain of Memories del 1.5+2.5 de PC que el jugador tenga instalado
(Steam o Epic) para reemplazar los videos de GBA y la música (mod base), y en un mod completo las
escenas de diálogo por las del 1.5, en todos los idiomas. El launcher detecta el juego instalado,
o una carpeta de instalación, o una carpeta con los videos y/o la banda sonora. Opciones on/off
independientes. Primero PC; Android y consolas si se puede. Apoyo: issue #25 (mapa de las 104
escenas a eventos de GBA, de DL-6-case): agradecerle al cerrar.

Nada del juego viaja con ReKHCoM: se lee la instalación del jugador.

## Lo que hay en la instalación (Steam: `steamapps/common/KINGDOM HEARTS -HD 1.5+2.5 ReMIX-`)

- Videos de Re:CoM: `STEAM/dt/KHCReSource/BIN/movie/*.mp4` (también KH1 en `STEAM/dt/KH1Movie`, BBS
  en `STEAM/juefigs/BBSReSource/movie/<idioma>/`). Están ofuscados: **solo los primeros 0x100 bytes**
  van con XOR de una clave fija de 16 bytes `3f1ec193264258b8aa9d5375bf629a97` (posición mod 16); el
  resto es un MP4 normal. ROP (apertura, 111 s), RSE (final de Sora, 353 s), RRE (final de Riku,
  345 s), RLP (157 s) en 1920x1080 H.264 + AAC; `zz_event_###` (100 escenas) en 1280x720.
  Audio en inglés; no hay pistas de otros idiomas.
- Música de Re:CoM: `Image/dt/Recom.hed/.pkg`, archivos `STREAM/0001/*.win32.scd` (45 pistas con
  nombre descriptivo, p. ej. `01F_Town_F`, `19_Alice_Field`). Paquetes: formato de OpenKh (OpenKh.Egs,
  Apache-2.0): `.hed` de entradas MD5(nombre)/offset/tamaños; en el `.pkg` una cabecera de 16 bytes
  que es la semilla de la clave, XOR sobre los primeros 0x100 bytes y zlib.
  Lector propio: `port/tools/khhd_pkg.py` (`list`/`get`). Nombres: `resources/recom.txt` de OpenKh.

## Slots de GBA (include/evt/mode_movie.h)

MOVIE_OPENING → ROP; MOVIE_ENDING → RSE; MOVIE_RIKU_ENDING → RRE; MOVIE_6F_GOAL y MOVIE_12F_E2 →
escenas del 1.5 según #25 (por confirmar mirando). RLP: bucle del menú de título (opcional).

## Diseño

1. **Importar (launcher, Windows primero):** detectar la instalación (bibliotecas de Steam en
   `libraryfolders.vdf`, Epic en sus manifiestos, o carpeta elegida, o carpeta con MP4/OGG ya
   sueltos). Copiar y descifrar a `<datos de ReKHCoM>/hd/movies/*.mp4` (mod base: ROP, RSE, RRE,
   ~1,2 GB) y `hd/scenes/zz_event_*.mp4` (las escenas: las de los finales con el mod base, todas con el completo, ~7 GB;
   desde el 23/10; una importación anterior las tenía en `hd/movies`, el launcher las mueve y el juego las
   busca en las dos) y convertir la música a `hd/music/*.ogg`. Lo importado
   es estándar: para Android se copia la carpeta `hd` al teléfono (el launcher de Android puede
   importarla con el selector de carpetas).
2. **Juego:** al pedir un video de GBA, si existe el MP4 importado y la opción está activa, se
   reproduce ese (Windows: Media Foundation; Linux/Android: decodificador a definir) con Start para
   saltar; si no, el video original. La música: tabla canción de GBA → pista de Re:CoM, decodificada
   con stb_vorbis, opción aparte.
3. **Mod completo:** al empezar un evento con escena del 1.5 (mapa de #25), reproducir esa escena en
   lugar del evento, con los subtítulos de Re:CoM en el idioma del juego
   (el audio es inglés). Opción aparte, apagada por defecto.
4. **Opciones (juego + launcher + ini):** `hd_movies`, `hd_music`, `hd_scenes`, independientes.

## Estado

- [x] Videos: ubicación y clave XOR (21/10).
- [x] Paquetes: lector `khhd_pkg.py`, música ubicada.
- [x] Conversión SCD → OGG (`khhd_scd.py`; en el launcher `hd_import.inc`, idéntica byte a byte).
- [x] Importador en el launcher: detecta Steam (registro + libraryfolders.vdf) y Epic (manifiestos),
  o carpeta elegida (instalación o archivos sueltos .mp4/.scd/.ogg); `ReKHCoM.exe --hd-import`.
- [x] Videos HD en el juego (Windows, Media Foundation): apertura, final de Sora, final de Riku.
- [x] Música HD (`port_hdmusic.c`): 43 canciones de GBA → pistas de Re:CoM, sigue pausa/parada/fundidos.
- [x] Mod completo (`hd_scenes`), 22/10: 97 de las 100 escenas asignadas (`match_hd_scenes.py`: diálogo en los
  cinco idiomas del cartucho contra los subtítulos de cada escena, más el orden de las escenas y los pares mudos);
  fuera: zz_event_001 (prólogo, dentro de la apertura que reemplaza ROP), 614 y 631 (solo Re:CoM). Antes: 72 eventos → escenas (filas confiables de #25,
  `port/src/port_hdscenes.inc`); al terminar, el evento se cierra con el salto de escena del juego.
  Audio en inglés (el PC no trae otras pistas).
- [x] Subtítulos (`hd_subtitles`, encendido): sí vienen, en `Recom.pkg` como `FORM/5xxx/FO5xxx.BIN`
  (tabla MVS en inglés US + remasterizadas UK/FR/GR/IT/SP). `khhd_subs.py` y el launcher escriben
  `hd/subs/<video>.<idioma>.txt` (581 archivos, idénticos byte a byte); el juego los dibuja bajo la
  imagen en su idioma (KHMenu, contorno oscuro). Algunas tablas descomprimen más de lo que dice su
  cabecera: se conserva todo (21/10).
- [x] Reproducción fluida: el video sale en NV12 del decodificador y la GPU lo convierte (textura NV12
  BT.709). Pedir RGB a Media Foundation hacía la conversión por CPU y los videos iban lentísimo. El
  plano de color se ubica por las filas reales del buffer (1088 en H.264 1080p); con 1080 el color
  quedaba corrido hacia abajo (reporte del usuario, 21/10).
- [x] Opciones en el juego (filas visibles solo si se importó) y en el launcher.
- [x] Música HD en las imágenes con puente (Android 64 bits, Switch, Vita, PS4, PS5), 21/10: la imagen
  no tiene libc para stb_vorbis, así que `port_hdmusic.c` conserva la lógica del juego (qué canción,
  pausa, fundido, ganancia) y en esas imágenes llama a la entrada nueva del puente `hd_music(op, ruta,
  ganancia/1000)` (0 abrir, 1 sonar, 2 pausa, 3 cerrar). Cada host incluye
  `port/ps5/shared/hd_music_host.h`: lee el .ogg/.loop con sus propios H_fopen/H_fread, decodifica con
  stb_vorbis y mezcla la pista en cada bloque que el juego empuja (`H_audio_push` = `HdMusicPush` sobre
  el push original, que ahora se llama `GamePush`), remuestreando lineal de 48 kHz a la tasa del juego.
  Sin hilos: sigue el reloj de audio del juego. PC y Android 32 bits siguen con su stream SDL propio.
  Una pista que no abre se intenta una sola vez por canción (antes se releía el archivo cada cuadro).
  Compilado: PC, APK (arm64 + armv7), host PS4. Probado en el emulador Android 14 con el host de 64
  bits (`test_android_scenarios.py --groups battles --only battle-000- --slow 6`, con
  `files/hd/music/01F_Town_B.ogg/.loop`: el combate de Ciudad de Paso pide esa pista): abre a 48 kHz,
  "song 12 -> 01F_Town_B", termina sin fallos (21/10). Switch (.nro) y Vita (.vpk) compilan en la VM (21/10).
- [x] Android: botón "Importar mod HD" en el launcher (selector de carpeta SAF; se elige la carpeta `hd`
  del launcher de PC o la que la contiene) que copia `hd/music` (.ogg/.loop) a `files/hd/music`, cada
  archivo entero (temporal + renombrar). Los videos todavía no: Android no los reproduce y pesan GB.
  Probado en el emulador Android 14: Descargas/hd → 2 pistas, idénticas por md5 (21/10). Android no deja
  elegir la raíz de Descargas: hay que elegir `hd` (el texto del botón lo dice).
- [x] Videos en Android (32 y 64 bits), 21/10: `port/portable/hdmovie_android.c` (MediaExtractor + MediaCodec a un
  AImageReader, subtítulos con FreeType, dlopen: Android 7+), entrada `hd_movie` del puente en el host de 64 bits, el
  importador copia videos y subtítulos. La imagen va a RGB en la CPU (tablas, un par de píxeles por croma) y se sube
  como textura común: las texturas YUV de SDL colgaban el emulador al crear su shader, así que quedan solo con
  `KHCOM_HDMOVIE_YUV=1`. Verificado en el emulador Android 14 (host de 64 bits): la apertura ROP 1080p completa con
  subtítulos, colores correctos (`KHCOM_HDMOVIE_SHOT=20`); el emulador traduce ARM y descarta cuadros, un teléfono no.
  Prueba: `KHCOM_MOVIE=1` con `files/hd/movies/ROP.mp4`.
- [ ] Videos en consolas y Linux (documentado 22/10; no entra en la v2.0: sin hardware para verificar). En esas
  plataformas el juego pasa sus propios videos y las escenas siguen siendo las del cartucho; la música HD sí suena.

  Lo que ya sirve a todas: la imagen de 32 bits pide el video al host por la entrada `hd_movie` del puente
  (`port/ps5/shared/bridge_list.h`), la misma que usa el host de Android 64. Del lado del juego solo hace falta
  compilar el camino `PORT_ANDROID64` de `port_hdmovie.c` también en las imágenes de consola (hoy devuelve 0) y que
  el host implemente `H_hd_movie` (hoy 0 en `ps4_host.c`, `switch_host.c`, host PS5). El host hace lo que hace
  `hdmovie_android.c`: abrir el MP4, decodificar H.264 + AAC, pasar NV12 a RGB con las tablas de ese archivo
  (si la consola no tiene texturas YUV), dibujar subtítulos (FreeType ya está en los hosts por el texto HD),
  fundidos de 0,3 s, pausa con Start, salto con su fundido, y devolver 1 (2 si se cerró la app).

  | Plataforma | Decodificador | Notas |
  |---|---|---|
  | PS4 / PS5 | `sceAvPlayer` (OpenOrbis lo trae: `libSceAvPlayer`) | entrega NV12 y PCM; el host PS5 corre la misma imagen; verificar en consola |
  | Switch | no hay API pública de nvdec en libnx: FFmpeg con aceleración `nvtegra` (la usan Switchfin y NXMP) | LGPL, varios MB en el .nro |
  | Vita | `sceAvPlayer` / `sceVideodec` (AVC) | los videos de Re:CoM son 720p (ROP 1080p): hay que recodificar ROP o bajar a 960x544 al importar |
  | Linux (i386) | empaquetar libavcodec/libavformat i386 (decenas de MB) u openh264 + minimp4 + un decodificador AAC | decisión de tamaño del usuario |

  Cómo probarlo cuando haya hardware: `KHCOM_MOVIE=1` con `hd/movies/ROP.mp4`, `test_hd_scenes.py` adaptado al
  host (log "hd movie: ..."), y el reporte de quien lo juegue.

## Otros pedidos para v1.9.21 (anotados acá para no perderlos)

- Texto HD: que la "a" minúscula no se confunda con la "o" y que la "r" se note más.
- Android de 64 bits y batería: `ANDROID_64.md`.
- Al final: todos los issues abiertos resueltos y respondidos (#22, #25, #33, #34…).

## Gráficos HD (cartas, HUD de combate, logo del título) — 22/10
- Los hace el importador (launcher de PC `Importar mod base`, o `ReKHCoM.exe --hd-import <juego> --sprites-only`;
  referencia `port/tools/khhd_sprites.py`) desde `Recom.pkg`, con la receta `port/data/hd_sprites.tsv`: 151 PNG en
  `hd/sprites` (~10 MB). Nada viaja con ReKHCoM.
- Android: el launcher copia `hd/sprites` junto con la música (carpeta `hd` traída del PC).
- Consolas (Switch, Vita, PS4, PS5): el juego los usa si `hd/sprites` está en su carpeta de datos (copiarla del PC);
  se dibujan por el puente con el host (`SDL_RenderGeometry` del shim). Presupuesto de texturas 12 MB en Switch y
  Vita (32 MB en el resto). Web: sin importador.
- Opciones: `hd_cards` (cartas, retratos y palabras de combate), `hd_title` (logo del título, intro y créditos).

## Calidad de los videos al importar (23/10)

El launcher recodifica con Media Foundation (Windows, sin instalar nada): H.264 High, VBR sin límite, 2 B-frames,
calidad 100, encoder de hardware; AAC 128 kb/s. `ReKHCoM.exe --hd-import <origen> --scenes --quality N` (0..5:
0-2 1080p, 3-5 720p; × Fuente / Buena / Ahorro). La elección queda en `hd/quality`. Bitrates: 1080p 10 / 5 / 2,5 Mb/s,
720p 5 / 2,5 / 1,2 Mb/s (nunca más que la fuente; Fuente sin cambio de tamaño = copia tal cual).

| preset | ROP (apertura 1080p) | zz_event_012 (escena 720p) | mod completo |
|---|---|---|---|
| 1080p Fuente | 143 MB, — | 56 MB, — | ~7 GB |
| 1080p Buena (sugerido PC) | 70 MB, VMAF 94,9 | 29 MB, 97,1 | ~3,3 GB |
| 1080p Ahorro | 40 MB, 89,5 | 15 MB, 91,4 | ~1,6 GB |
| 720p Fuente | 69 MB, 93,7 | copia | ~6 GB |
| 720p Buena (sugerido Android) | 36 MB, 90,5 | 29 MB, 97,1 | ~3 GB |
| 720p Ahorro | 18 MB, 82,5 | 15 MB, 91,4 | ~1,5 GB |

**Sin 720p (usuario 24/10):** la opción de resolución se quitó: las escenas ya vienen en 720p, así que 720p
Buena pesaba 3,5 GB contra 3,7 GB de 1080p Buena (6 %; el usuario pidió quitarla si no ahorraba al menos 20 %).
Quedan tres compresiones a la resolución de origen: Fuente (6,7 GB, medido), Buena (3,7 GB, medido), Ahorro.
`hd/quality` y `--quality` viejos (3..5) valen como 0..2.

**Comprimir lo instalado (24/10, pedido del usuario):** botón del launcher (Sonido → Mod HD) y
`ReKHCoM.exe --hd-compress --quality N`: recodifica en el lugar los videos y escenas ya instalados (p. ej. un mod
completo importado en Fuente) a la calidad elegida, sin la instalación de Re:CoM. Deja tal cual lo que ya está a esa
calidad o menos (comparando altura y bitrate; recodificar otra vez solo pierde detalle) y lo que no queda más chico;
cada archivo pasa por `nombre.source.mp4`, que vuelve a su nombre si el launcher se cerró a mitad. Probado: ROP
140 MB → 69 MB (Buena) → 41 MB (Ahorro); repetir la misma calidad no toca nada; un corte a mitad se recupera.

VMAF contra la fuente a su tamaño (el 720p de la apertura escalado a 1080p: cuenta la pérdida de resolución).
Probado: Main/CBR 96,8 y el encoder de software 94,2 contra 97,1 del elegido a igual peso.
