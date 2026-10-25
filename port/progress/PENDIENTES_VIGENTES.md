# Pendientes vigentes hacia la v2.0 (lista limpia, 25/10/2026 noche)

## Meta vigente del usuario — 25/10/2026

Ver [META_V2_0.md](META_V2_0.md): arena Trickmaster → escena099 →004 → Síntesis (carta HD dinámica) → Añadir naipes (cantidades detrás del selector); después el resto de piezas comentadas y verificaciones técnicas vigentes. Este orden sustituye órdenes históricos. PR111/114 integrados; propuestas nuevas121/122/123/124 en Atlas y en revisión, no cerradas por subirlas. El documento conserva las mejoras futuras fuera de bloqueantes por defecto. Publicar la estable requiere OK.


Única cola de trabajo. Lo anterior completo (traspasos, borradores, detalle): **archivo/PENDIENTES_ARCHIVO_20261025.md**
y archivo/PENDIENTES_ARCHIVO_20261024.md. Método de depuración: DEBUG_WORKFLOW.md. Protocolo de la cola:
COLA_COLABORATIVA.md. Estado vivo de piezas: `python port/tools/art_queue.py --quiet list`.

## Reglas del usuario (completas en REGLAS_DEL_USUARIO.md)

- Rama y PR por entrega; lo visual pasa por la app de revisión (Atlas) y solo se integra lo aprobado.
  Lógica aprobada: LOGICA_APROBADOS.md (cada PR de arte la mejora).
- La validación en hardware real no es pendiente: llega como reporte. Publicar (releases, comentarios) solo con OK.
- Nunca borrar carpetas ReKHCoM-vX ni ningún release. Trabajar solo en KHCoMPC (C:) y D:/01Claude/ReKHCoM-build.
- Toda falla o timeout de batería es un bug: reproducir. Notar cada avance aquí al momento.

## Estado (25/10 noche)

- Traspaso activo de Codex (arte aprobado, ZIP local): TRASPASO_CARGAS_ZIP_20261025.md.
- Propuestas Atlas abiertas: #121 (Trickmaster), #122 (099), #123 (004/005), #124 (selector HD), más #120 (esta meta). Última copia interna: **v2.0-pre15** (zip limpio en KHCoMPC + `jugar/ReKHCoM-v2.0-pre15`).
- Hoy en main: Opciones por secciones (solo A para entrar, Salir del juego, mods en gris con cartel), logros Re:CoM
  con borde e insignias, #83 (carta de Jafar: BIOS_PTR), mazo sin parpadeo en invocaciones, videos HD en Linux,
  #42 (3 pistas HD de eventos), arenas aprobadas (Maléfica, Úrsula, Marluxia, Trickmaster, Oogie, Darkside, 112, 120).

## 1. Código (Claude)

- **#83 sin reproducir en PC:** 24 peleas largas con Jafar (build anterior a #115, con y sin HD, recogidas forzadas):
  0 fallos y nunca una recogida con la selección vacía. El código lo explica: todo lo que vacía `sSoraSelectedCard`
  marca una recarga, y la recogida espera a que no haya recarga. #115 queda como protección, no como causa probada.
  Siguiente: `test_android_scenarios.py --groups jafar83` en el emulador (build nueva y anterior) o el `kh_com.log`
  del jugador. El issue sigue abierto.
- **Android (con el emulador de las baterías):** repetir la batalla 123 US (timeout; en PC no se reproduce),
  rendimiento en gama baja (línea `timing:`), tablets con el encuadre nuevo, botones táctiles (también web),
  elección de modo al primer arranque.
- **Baterías:** `run_batteries.py --status` en `khcom-claude-int` (relanzadas 25/10 noche): combates EU y luego
  Android; después la pasada `--hd`.

## 1b. Próximo (pedido del usuario 25/10 noche, en este orden)

- **Pre16 para el jugador de #83:** zip de Windows y APK listos (`build/releases` en khcom-recom-art, main 0f415d7a3).
  Falta el OK del usuario para publicar la prerelease y comentar en #83 (la herramienta pide confirmación).
  En el emulador Android 64 (arm64 traducido) `--groups jafar83` EU/US: 4/4 sin cierre con la build nueva.
- **Galería de escenas (nueva función):** en Opciones > Re:CoM, repetir las escenas de historia ya vistas; pestañas
  Normales / HD (HD solo si están cargadas); lo no desbloqueado como «???», como el diario de Pepito Grillo; el avance
  es el de todo el save de ReKHCoM (lo más lejos que se llegó en cualquier partida).
- **Mods y mejoras en todas las plataformas:** hecho el mod HD en un archivo (#132: launcher de PC > «Llevar el mod
  HD a otra plataforma», el juego o su host lo instala solo; README por plataforma). Falta:
  - Android: comprobar en el teléfono/emulador que «Instalar ROM...» con ReKHCoM-HD.zip instala el mod.
  - Vita y PS5: confirmar sus builds con el paquete (Switch .nro y host PS4 compilan).
  - Videos HD en consolas (decodificador en cada host, HD_MOD.md: sceAvPlayer PS4/PS5/Vita, FFmpeg nvtegra Switch).
  - Texto HD en consolas (`port_hdtext` excluido: FreeType del lado del host por el puente).
  - Estados rápidos/revanchas en consolas, Android y web (`port_state` excluido u oculto).
  - Que el ZIP de cada consola traiga su README con la sección «HD mod».

- **Hecho 25/10 noche:** #132 mod HD en un archivo (PC, Linux, Switch .nro, Vita .vpk, host PS4 compilan; Android
  64 probado en el emulador: el zip queda en files/hd), #134 Android «Instalar ROM...» acepta ReKHCoM-HD.zip,
  #133 launcher: cuenta RetroAchievements siempre visible con emblema propio, #128 builds de guests reparadas.
- **Logros ganados antes de vincular la cuenta (pedido del usuario):** no subirlos solos. Las reglas de RA solo
  aceptan desbloqueos sin conexión guardados por un emulador con sesión iniciada; desbloquear «saltando» el código
  puede dejar la cuenta Untracked. Propuesta: el launcher lista los logros locales ganados sin cuenta (fecha y
  hora) y explica cómo pedir el desbloqueo manual en retroachievements.org. Confirmar con el usuario antes.
- **Launcher:** emblema de RetroAchievements hecho (propio, no el logo oficial por ser marca registrada).

## 1a. Cola del usuario 25/10 noche (en curso; hechos marcados)

- [x] Atlas: «Necesita mejoras» = **Aprobado (mejorar)**; todo lo aceptado va al juego (#138). Falta reiniciar el
  Atlas en vivo (servidor de khcom-review-app) para que lo muestre.
- [ ] Escenas aceptadas con receta encendidas (rama `claude/scenes-approved`: 007, 044, 045, 087, 090, 095, 098, 099,
  104, 110, 112, 118, 120, 124). Pruebas en D:/01Claude/ReKHCoM-build/prelim-old|new (pre17 contra la rama): integrar
  si los fallos de la rama ya estaban en la pre17 (057, 107, 124 fallan igual en pre17).
- [x] Opciones: sección HD (Texto HD con subtítulos, A = ventana de fuente con muestras, Intros, Música, Cinemáticas
  con cartel, HUD HD; texto HD por defecto salvo GBA clásico) #141; fuente solo KHMenu/Sistema #139.
- [x] Repo limpio (#137), baterías se podan solas (#142), C:/D: liberados (C 14 GB, D 16 GB libres).
- [ ] **Bonus élite** no se adapta a pantallas anchas (cabecera y barras a ancho GBA, cartas salen por abajo): revisar
  el diseño del menú en `port_hdlayout`/HD menus para todos los aspectos (16:9 a 32:9).
- [ ] **Diario, páginas de Historia (tercer nivel):** capturarlas (KHCOM_KEYS hasta Diario > Historia > entrada con una
  partida con historia) e importarlas a Atlas como piezas nuevas (import_image.py), para las marcas del usuario.
- [ ] **Filtro de audio:** apagado por defecto; rehacer en dos perfiles: sonido 16 bits (juego) y música HD moderna.
- [ ] **Logros sin conexión con cuenta:** guardar en disco los desbloqueos hechos con sesión y sin red, y subirlos al
  volver (RA lo permite: cola segura de un cliente con sesión). Hoy rc_client los reintenta solo mientras el juego vive.
- [ ] **Banco de partidas:** cada banco con su primer arranque (idioma, modo, pantalla completa/ventana), sus ajustes y
  sus logros locales (avisos de nuevo aunque haya cuenta); traer de la cuenta los que falten en local.
- [ ] **Primer arranque:** preguntar pantalla completa o ventana con el estilo de la elección de modo HD/GBA.
- [ ] **Título en HD** hasta cargar la partida, y Opciones en HD cuando el HUD HD está activo (diseño; ir a Atlas).
- [ ] **Galería de escenas** (ver §1c).

## 1c. Traspaso de las funciones nuevas (cómo abordarlas)

- **Galería de escenas** (Opciones > Re:CoM): los eventos de historia ya llevan su piso en el nombre
  (`include/evt/event_ids.h`: `EVENT_0XX_<n>F_ENTRANCE/GOAL`, `RIKU_B<n>F_...`) y la partida guarda el avance por piso
  (`GameState.floors[13]`); un evento está desbloqueado si alguna partida del save (todas las ranuras y bancos,
  `PORT_SAVE_BANKS`) pasó ese piso con ese personaje. Reproducir: el arnés ya lo hace (`gPortEventTest` →
  `PortEventSelectPreset`, src/main.c:1014); hacer lo mismo desde Opciones y volver al juego (como «Volver al título»
  pero restaurando el estado: guardar un estado rápido antes, `port_state`, y cargarlo al terminar). Pestaña HD: la
  tabla `kHdScenes` (port/src/port_hdscenes.inc) da el video de cada evento; solo si `PORT_ASSET_HD_SCENES`. Lo no
  desbloqueado: «???» con el estilo de las filas del diario.
- **Logros ganados antes de vincular RA:** ver §1b (política); listar `achievements_official_eu.dat` con fecha contra
  los del perfil (`ra->account_unlocks` en ra_client.c) y mostrar los que faltan con un enlace a la ayuda de RA.
- **Sesión de RA por usuario del PC** (#135): hecha; falta verla con una cuenta real (dos copias en dos carpetas).
- **Videos HD en consolas:** el puente ya tiene `hd_movie` (lo usa Android 64): compilar el camino `PORT_ANDROID64` de
  port_hdmovie.c en las imágenes de consola e implementar `H_hd_movie` en cada host (HD_MOD.md: sceAvPlayer en PS4/PS5,
  sceAvcdec en Vita con ROP a 960x544, FFmpeg nvtegra en Switch). Necesita consola real para verificar.
- **Texto HD en consolas:** PS5 ya tiene `port/ps5/guest/src/hdtext_ps5.c`; revisar el resto (Switch/Vita/PS4) a partir de él; `port_hdtext` (GDI) está excluido; o `hdtext_freetype.c` del lado
  del host (FreeType ya está en los hosts SDL2) con una entrada nueva del puente que devuelva la cobertura de una
  línea (como `PortPortableHdTextCover`).
- **Estados rápidos en consolas/Android/web:** `port_state` excluido en consolas; en Android/web la fila de revanchas
  se oculta. Medir el tamaño del estado y escribirlo por el puente (H_fopen) antes de encenderlo.

## 1d. Después de la pre18 (pedido del usuario 25/10 noche)

- **Modos Re:CoM basados en la versión de PS2 (siguiente trabajo grande):** dificultades, ajustes por enemigo y lo
  demás que se encuentre, con sus variantes, aplicados con cuidado a nuestra versión y reimplementados donde haga
  falta (dentro de la lógica del juego, sin objetos del port). Usar los assets de 1.5 HD ReMIX donde aplique.
  Herramienta: **REA** (github.com/morluto/rea, MIT; `npx rea-agents setup`, Node 24 ya instalado) con Ghidra o
  objdump/Rizin para el ELF de PS2 (MIPS R5900; REA no nombra PS2, probar que lo abra). Lista de referencia:
  github.com/yibie/awesome-rea (way-of-the-samurai-decomp es el caso PS2 más parecido). Falta el material del
  usuario: ELF/ISO de Re:CoM PS2 y la carpeta de 1.5 HD (no están en KHCoMPC/roms). Cómo empezar: sacar de REA las
  tablas de dificultad (multiplicadores de daño/PV/experiencia por modo) y de enemigos, compararlas con las de la GBA
  (src/), proponer al usuario qué entra en cada modo antes de tocar el juego.
- **Potencia en cada plataforma (pedido 25/10):** Linux/Steam Deck: la build nativa es i386 y dibuja por software;
  el camino con GPU es un host Linux de 64 bits con el guest x32/reloc (como android64_host/PS5); mientras, Proton
  con la build de Windows (usa la GPU) y la prueba `REKHCOM_GPU=1`. Consolas: solo relojes documentados y de uso
  común (Vita ya pide 444 MHz; Switch: perfiles de reloj oficiales del sistema). Revisar cómo lo hacen otros ports
  de decomp (sm64ex, SoH, Perfect Dark port) antes de tocar nada.

- **iOS (para la línea 2.0, puede salir después de las demás):** evaluar una IPA para instalar por fuera de la
  App Store (ESign, AltStore, SideStore, Sideloadly). Referencia del usuario:
  github.com/DevZer0D4Y/TheSimpsonsGame-iOS. Revisar también los repos con estrella del usuario que hagan lo mismo.
  Punto de partida: el guest de 32 bits va por PORT_GBA_RELOC como en Android 64 (host arm64 + shim SDL), el host
  iOS con SDL3 y Metal; firmar sin cuenta de pago (la firma la hace la herramienta del usuario). Usar la potencia de
  iOS: Metal, 120 Hz (ProMotion), mandos MFi.

## 2. Arte (Atlas: GPT/Codex; Claude ayuda)

- Responder en la app cada envío en turno «agente» con una versión hija (`COLA_PARA_AGENTES.md`).
- Piezas de la cola: 057, 061, 074, 097, 081, diario (entradas y subentradas), Maléfica 01-03.
- Jafar: la columna negra de la subida de nivel y el recuadro de la escena previa son del cartucho (también con
  `KHCOM_JAFAR_WIDE=0`): rellenarlos es arte dentro de la imagen GBA, solo si el usuario lo pide.
- HUD HD a lo Re:CoM: lo que no tiene equivalente 1:1 (contador CARDS, título) es decisión de diseño.

## 3. Esperando reportes (no se trabaja hasta que lleguen)

- #83 confirmación del jugador; Proton estable (`kh_com.log`); cara HD de Sora con el pelo cortado al recibir daño;
  cuadro negro en Agrabah con HUD HD (falta captura); Vita: undub por cabecera y velocidad.
- Si un issue cerrado recibe comentarios: responder y volver a cerrarlo. Sin plataforma/versión: preguntar.

## 4. v2.0 estable (solo con OK del usuario; no saltear nada)

- Sin Switch (sacar el `.nro`). `test_update.py` contra `respaldos/releases-estables` y `--olds respaldos/releases-prueba`.
- No se borra ningún release ni archivo; README con la versión; copia de la estable en `respaldos/releases-estables`.
- `release_check.py v2.0` antes y `--post` después; `check_sources.py`.
- Builds desde el commit final: Windows, Linux, Android, PS4, Vita, PS5 (VM), web.
- `build/smoke_pc.sh` con cada ejecutable de PC; `test_hd_audio.py`, `test_hd_audio_flow.py`, `test_frameskip.py`,
  `test_hd_scenes.py`, `match_hd_scenes.py --check`, `test_launcher.py`.
- Copia de prueba + `.bat`; notas cortas que anuncien el mod HD; nunca decir en público que algo no se probó en
  hardware. Al publicar, responder #83 si quedó confirmado.

## 5. Después de la v2.0 (mejoras, no pendientes de ahora)

- Switch (SWITCH_MOTOR.md); Vita/PS4 a la GPU (RENDIMIENTO.md); PPU de GBA en varios núcleos.
- Online (~70 %, ONLINE_ESTUDIO.md); menús Re:CoM en HD (HUD_HD_RECOM.md); rutas de Sora y Reverse/Rebirth (#39).
- Mejoras de lo «aprobado con mejoras».
