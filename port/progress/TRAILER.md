> **Encargo vigente: tráiler v2.0.** Ver [TRAILER_V20.md](TRAILER_V20.md).
> Este documento conserva las directrices y el historial del tráiler anterior (juego v1.8.2).

# Tráiler v4 retomado por Codex — v1.8.2 republicada

- [x] Tomas nuevas de juego, audio y launcher con la build pública d7b95a8cd.
- [x] x4 explícito, dos arenas 32:9, seis idiomas y opciones internas.
- [x] Intro sin consola: 240x160 nativos, expansión, slider y cámara compartida.
- [x] Paseo de castillo largo sin congelar su final; música desde el slider.
- [x] Audio final termina antes de los poscréditos Android.
- [x] Export final revisado, decodificado y comprobado contra la fuente.

Entrega y límites de verificación: TRAILER_V4_ENTREGA.md.

Trabajo nuevo separado en ../Trailer/v182 (se conserva el material anterior).
Scripts portables en port/tools/trailer/README.md. ZIP del juego ya publicado;
el vídeo no modifica sus ejecutables ni el digest del paquete.

---

# Tráiler de ReKHCoM — cómo se hizo y qué falta (v4)

Herramientas: el propio juego (grabación y audio), ffmpeg 9 y Python+Pillow.
Sin editor de vídeo ni IA. Todo está en `../Trailer/` (fuera del repo):

- `Trailer/tools/` scripts (antes vivían en el scratchpad de Claude):
  - `fx.py` arnés: copia `build/<build>/khcom.exe` a `build/fx_<nombre>/`, escribe
    `kh_com.ini`, lanza con drivers dummy y sin mandos (SDL_JOYSTICK_*=0, para no
    leer el mando del usuario), y devuelve los BMP de `KHCOM_SHOTS`.
    `COPYSTATE=<.khst>` copia un estado y se carga con `KHCOM_LOADSTATE`.
  - `trailer_rec.py` graba cada clip con `KHCOM_RECORD=desde-hasta:ruta.mov`
    (ProRes 1080p60 por ffmpeg, lee la ventana). Lista `jobs`: idioma, diario,
    entrada a combate, 9 arenas (`KHCOM_BATTLE=10:<arena>:0` + `KHCOM_MONKEY`
    para que Sora pegue al azar), salas por mundo (`room:1WW01` andando UP),
    sala de Monstruo saltando el barril (`05_sala_mundo_3_salto`), 32:9
    (Agrabah arriba, Halloween abajo, ventana 2560x720), película, tienda
    texto píxel/HD, idiomas, castillo 4:3 y 16:9 (`00c`/`00d`), menú de
    opciones (`15_menu_opciones` con `KHCOM_VPAD`).
  - `sfx_rec.py` re-graba cada clip solo con efectos (música a 0) y
    `KHCOM_WAVDUMP`; los WAV van a `Trailer/audio/sfx/`. La música va aparte
    (`Trailer/audio/m_titulo.wav`, `m_combate_agrabah.wav`).
  - `cards.py` títulos (Palatino, brillo, línea dorada) -> `tools/cards/`.
  - `sweep.py` barrido texto ORIGINAL/HD; `console.py` (descartado: consola
    genérica); `intro.py` monta la intro (quitar la consola, ver abajo);
    `postcred.py` post-créditos Sincorazón "ANDROID RELEASE / COMING SOON"
    (ya hecho: `clips/16_postcreditos_android.mov`).
  - `edit.py` motor de montaje (xfade, transición corazón `heart_expr.txt`,
    slider `slider_expr.txt`, zoom/push-in, tarjetas `card`/`card2`),
    `acts.py` los actos A..F (v4), `final.py` une los actos y mezcla música
    + efectos con loudnorm -14 LUFS. `acts_v3.py`/`final_v3.py` = la v3.
- Salida: `Trailer/ReKHCoM_trailer_v4.mp4` (v3 = `_final.mp4`, publicada).

Orden: build release actual -> `python trailer_rec.py` -> `python
sfx_rec.py` -> `python intro.py` (necesita 00c/00d) -> `python sweep.py`
-> `python cards.py` -> `python acts.py A B D E F` -> `python final.py`.
Revisa con hojas de contacto (ffmpeg select+tile), 3-4 por tramo, no más.

## Mejoras pedidas por el usuario para la v4 (pendientes)

1. Grabar TODO con la versión publicada más nueva (v1.8.2), zoom x4.
2. Intro (SIN consola: la idea de la GBA dibujada queda descartada).
   Escena del Castillo del Olvido (blanco, contraste) con Sora andando.
   Empieza en la resolución original de la GBA: el cuadro 240x160 en su
   proporción (clip `00c_castillo_gba_4x3`, pixel exacto, centrado en negro)
   con "2004 · Game Boy Advance". Crece rápido hasta llenar la altura y el
   SLIDER (como el de texto ORIGINAL/HD) lo abre a la vista 16:9 de nuestro
   port a pantalla completa (clip `00d_castillo_pc_16x9`, mismo momento del
   paseo); se queda >=3 s en 16:9 con "2026 · Native on PC". No usar
   `console.py` ni `cards/console.png`; `intro.py` hay que adaptarlo
   quitando la capa de la consola.
3. La música empieza justo cuando pasa a 16:9 (T_REVEAL en `final.py`) y
   sigue hasta el nombre del proyecto, como antes.
4. Salas: progresión encadenada — cortar de una sala a la siguiente con
   Sora en la misma fase del movimiento (p.ej. a media escalera en una y a
   media escalera en la otra). Monstruo: usar el clip saltando el barril.
5. Final como la v3 (logo, disclaimer, música que se apaga) + post-créditos
   de los Sincorazón (ojos amarillos, emblema, "ANDROID RELEASE / COMING
   SOON"), sin música.
6. Mostrar el menú de opciones interno (clip 15) y el launcher.
7. Sin capturas sueltas (nada de World Warp ni frames congelados de la
   película al final). Transiciones como la v3 (destellos, corazón, slider).

## Guion (estructura actual en `acts.py`, ~1:15)

Actos unidos en `final.py` con estas transiciones entre ellos: A->B corazón
(`heart`, 1 s), B->D negro, D->E destello, E->F destello largo.

- **A — Intro y logo.** Castillo 4:3 GBA -> slider -> 16:9 (ver mejora 2),
  tarjetas `c01` "2004 / GAME BOY ADVANCE" y `c02` "2026 / NATIVE ON PC".
  Destello -> entrada a combate (el corazón que estalla, `03_...`) ->
  título del juego de fondo con `c03` "ReKHCoM / KINGDOM HEARTS CHAIN OF
  MEMORIES". La música (tema del título) entra al pasar a 16:9 y llega
  hasta el logo.
- **B — El mundo, más ancho.** `c04` "THE WORLD, WIDER". Salas de cada mundo
  (`05_sala_mundo_N`) encadenadas a media escalera (cortes en el mismo
  momento del paso), Monstruo saltando el barril. Luego `c05` "EVERY ARENA
  IN 16:9": ráfagas de combate de 1,5-1,9 s en distintas arenas, unidas con
  destellos blancos (`FL`). Música: tema de combate desde aquí.
- **D — Ultrawide.** Negro con `c08` "UP TO / 32:9" (apertura circular) ->
  2 pantallas 32:9 apiladas: Agrabah arriba, Halloween Town abajo, combate
  con Sora pegando (`07_widescreen_2x32x9`, línea separadora).
- **E — Detalles.** Película ampliada (`08a`), barrido texto ORIGINAL TEXT /
  HD TEXT con zoom a la caja (`09c_texto_barrido`), mosaico de 6 idiomas con
  nombres (`10_mosaico_idiomas`), menú de opciones interno con `c12` "EVERY
  SETTING, IN GAME" (`15_menu_opciones`, sin efectos de sonido), launcher
  flotando sobre su propio fondo desenfocado (`14b_launcher`) con `c09`
  "HD TEXT / 6 LANGUAGES / RE:COM MODE".
- **F — Cierre.** Título de fondo con `c10` "ReKHCoM / AVAILABLE NOW - FREE /
  github.com/MatyMeatBoy/ReKHCom"; negro con `c11` (disclaimer: fan project,
  no afiliado, ROM propia); la música se apaga ahí. Post-créditos en
  silencio: Sincorazón (`16_postcreditos_android`).

Textos de las tarjetas en `cards.py` (inglés, Palatino con brillo y línea
dorada, tercio inferior con degradado oscuro).

## Lo que ya se hizo y lo que el usuario dijo (historial)

- v1: corte completo 1:22 (`ReKHCoM_trailer_v1.mp4`). Pedidos: Sora debía
  pegar en las arenas (-> `KHCOM_MONKEY`), quitar comparativas 4:3 vs 16:9,
  intro más pro, mejor escena abajo en el 32:9, barrido de texto con
  rótulos, final menos cortado, más movimiento en las tomas.
- v2 (1:15): aplicado todo lo anterior. Pedido: la intro debía pasar de
  4:3 a 16:9 con Sora en el mundo (no la pantalla de idiomas) y mostrar el
  menú de opciones interno.
- v3 = `ReKHCoM_trailer_final.mp4` (1:11): intro en Ciudad de Paso con
  slider, menú de opciones, sin World Warp ni congelado final. El usuario
  dijo "casi perfecto".
- Pedidos para la v4 (este documento, sección de mejoras): Castillo del
  Olvido, de resolución GBA a 16:9 SIN consola, música al pasar a 16:9,
  salas encadenadas, post-créditos Android, grabar con la última versión.
- Extras listos: miniatura `Trailer/ReKHCoM_thumbnail.png` y descripción
  de YouTube `Trailer/YouTube_descripcion.txt` (actualizar capítulos/tiempos
  al terminar la v4).
- Cuidado: la música es del juego; YouTube puede reclamarla. Está en una
  pista aparte en `final.py` para poder cambiarla.
