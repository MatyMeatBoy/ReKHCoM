## v1.9 PUBLICADA — 17/10/2026

- Release publica estable Windows: https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v1.9
- Android Alpha3 ARMv7 actualizado adjunto a la misma release. Build EU/US/JP,
  parser48 y touch PASS; NO se comprobo en ARM fisico. Mantener etiqueta alpha.
- Web Alpha3: motores EU/US/JP nuevos; Wasm validado/compilado en Node, JS sin
  errores de sintaxis. Pages desplegado; verificacion HTTP/SHA en reporte adjunto.
  https://matymeatboy.github.io/ReKHCoM/
- Windows: PPU206, Riku7, launcher11/import6, fondos circular/rectangular, input
  opt-in/hold/foco/overlay y guardado aislado PASS. Paquetes sin ROM/save/config.
- Devolver acumuladas: runtime OFF inerte; ON devuelve1 y3 cartas asentadas y
  recupera mano21. Fixture3: KHCOM_BATTLE=0:1:0, modo200:hudhp:100, STOCK300/450/600,
  UNSTOCK760-800. Intento con300/360/420 solo acumulo2 por combate/animacion:
  no asumir que cada input del harness logra acumular una carta.
- Instalacion local del BAT actualizada; hashes de saves, INI y estados intactos.
  Copia del savestate anterior + EXE exacto guardada PRIVADA para diagnostico.
- Git main y tagv1.9 actualizados al cierre; snapshot previo preservado como tag
  v1.9-preview-20261017. Nunca subir fixtures privados de build/ ni arte extraido.
- Linux/Steam Deck/RG34XX siguen SIN binario nativo: ABI32/64 pendiente.
  Otros cheats/editor, menus relativos experimentales y zoomx3 reportado pendientes.
  No anunciar esas tareas como terminadas ni compatibilidad de dispositivos no probados.

## Cierre v1.9 autorizado (17/10/2026)

- Usuario autoriza publicar v1.9 tras corregir los bordes de Riku; sustituye
  la restriccion anterior de mantenerla privada. Windows estable; Android/Web alpha.
- Audio: usuario confirma que suena; prueba WASAPI con mezcla no silenciosa.
- Cargar: Riku SOLO con truco ON o Sora completado. 7 pruebas reales pasan,
  incluidos checksum de cabecera de fixture completado, OFF oculta sin borrar,
  ON carga/guarda Riku con las otras ranuras intactas. Original privado intacto.
- Devolver cartas acumuladas vuelve SOLO en Experimental, default OFF.
  Nueva clave experimental_unstock; ignora unstock_button antiguo y bit512 de
  metadatos. No incluido en paquete QoL de nueva partida. Mantener V / stick
  derecho pulsado o hacia abajo 0.5s, solo Sora; no cancela tecnica en curso.
  Procedencia: ggdrn/KH_COM_VITA (MIT), no una mecanica GBA/PS2 nativa.
- Riku: primer circulo corregido + base rectangular opaca de cinematicas:
  extiende bandas de tiles nativos; rechaza tiles parcialmente transparentes,
  menus, batalla y foreground. Original 240x160 exacto. Pruebas x1/x4, fade,
  paleta y rechazo de geometria PASS. Savestate usuario respaldado con su EXE
  exacto en build/riku-hollow-user-review (PRIVADO, no subir).
- Linux/Steam Deck/RG34XX siguen BLOQUEADOS por ABI punteros32/64; no hay
  binario nativo probado. No etiquetar ARMv7 como ARM64 ni inventar soporte.
- Menus relativos experimentales excluidos. Otros cheats/editor siguen pendientes.

## Politica vigente (17/10/2026)

Solo v1.8.3 publica estable. v1.9/web-alpha2 se ocultaron como borradores,
con tags conservados. No publicar candidatas ni actualizar Pages con cambios
sin validacion. Completar pendientes antes de siguientes versiones publicas.
Los apartados siguientes describen publicaciones historicas ya retiradas.

## v1.9 unificada y BAT local (17/10/2026)

- Usuario corrigio la organizacion: Windows y Android v1.9 en UNA release.
  Canonica: https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v1.9
- Android APK/sha/manifiesto adjuntos y digest verificado ANTES de borrar la
  release duplicada android-alpha2. Tag android-alpha2 conservado; no borrado.
- ZIP Windows publicado cd1df8f1d4d0f20b76bf965d88e819d809eaab6c9d0a931b805ed122b5a7d45d.
  Descargado desde GitHub: 11/11 launcher ROM,6/6 saves reales y6/6 ejecuciones
  motores EU/US/JP en4:3/3:2 PASS. v1.9-postpublish.json adjunto en release.
- BAT creado en carpeta madre: Probar ReKHCoM v1.9.bat. Script reproducible
  port/tools/prepare_local_release.ps1 verificaZIP, instala en carpeta propia,
  copia ROMs verificadas y migra SRAM/config de versiones anteriores sin pisar
  archivos existentes. Probado tambien con powershell.exe (Windows PS5).
- Web compilando EU/US/JP para actualizar Pages y adjuntar ZIP a MISMA release
  v1.9. No crear otra publicacion duplicada por plataforma.
- Usuario pidio quitar Game language de la web: eliminado selector y argumento
  --language. El juego elige idioma; selector English/Espanol del sitio permanece.
  Test frontend verifica arranque sin idioma forzado y ausencia de selector redundante.
- v1.9.1 sigue siendo objetivo final estable CON TODAS las plataformas. No esta
  lista: Linux64 RG34XX/Deck, cheats/editor y validacion fisica siguen pendientes.

## Pedido actualizado y candidata PC v1.9 (17/10/2026)

- Usuario pide publicar v1.9, revisar paquetes publicados y corregir, y luego
  v1.9.1 publica estable UNIFICADA para todas las plataformas. Esto sustituye
  el criterio anterior de no-PC siempre alpha. No marcar v1.9.1 cerrada solo
  por acabar PC: Linux ARM64 RG34XX y Steam Deck aun carecen de build.
- Rama release/v1.9 integra feature/aspect-43-zoom + fix/web-mobile-files-controls.
  La escala relativa nueva de menus sigue EXCLUIDA por instruccion del usuario.
- PC EU/US/JP construidos. PPU206, audio24 +800000 integrales, zoom41, padding810,
  stock3regiones, teclado/mando/touch y frontend Web PASS.
- Regresion regional: 18 imagenes SDL exactas vs stable v1.8.3 (campo/combate/deck,
  1920x1080 y2560x720). EXP:20 configuraciones,3 estados; HD no altera los pixels.
- Menu HD:4 ciclos reales de barajas/editar y vuelta campo PASS. Estado: NEW no
  invade margenes x1/x4 en1920x1080 y5120x1440. No equivale a resolver el antiguo
  solapamiento HD de Barajas sin reproduccion clara.
- Bugs nuevos corregidos en launcher: conversion borraba flags en0x1c en vez de
  idioma en0x1e; import ignoraba fallo del backup y truncaba destino directamente.
  Ahora parser limitado y validado, eleccion de base SRAM por bloques completos,
  backup obligatorio y reemplazo atomico con flush. 96 checks C++ PASS; launcher
  real con bancos/backup fallido/destino bloqueado/cabecera rota/.srm envuelto PASS.
- El fallo inicial de test_launcher con --out relativo era del fixture (ruta
  DECOMPUI_INSTALL relativa al cwd hijo); normalizado resolve(),11/11 PASS.
- El primer test_menu_cycles uso una SRAM de importacion sin ranura jugable:
  NO era una regresion del motor. Repetido con copia jugable banktest1,4/4 PASS.
- Pendientes: empaquetar/publicar candidata, descargar/revisar; cheats/editor,
  Linux64/Deck, comprobar Android fisico y web real movil; 1.9.1 aun no cerrada.

## Web Alpha 2: cierre F11 y artefactos finales (16/10/2026)

- F11 ahora pide fullscreen del canvas (igual Alt+Enter), previene repetición y
  deja Escape al navegador. No depende solo de ocultar barras de Chrome. Test
  de producción JS también prueba la petición F11 y el callback de resolución.
- Código final 292080d89; Pages ac7dd3c372160f04fb2e48d77eba79c54d935468.
  Nueva verificación pública 17/17 hashes PASS. WEB_PUBLICACION.json actualizado.
- Tag web-alpha2, creado en esta tarea, se actualizó con lease desde ca86d0016 a
  292080d89 para incorporar F11. No se cambiaron tags anteriores ni Windows.
- ZIP final 4.386.176 bytes, SHA256
  0134d45a734e51f8237ed4ea8c851a72d4257aff28d7022749951e8deb3358fc.
  Assets reemplazados en el mismo prerelease. Supersede hashes de Alpha2 previos.
- El usuario debe exportar su partida y recargar una pestaña vieja para ejecutar
  el motor nuevo. No intentar cambiar un Wasm activo durante una partida.
- Conserva pendientes de gameplay real regional, F11/Escape manual, persistencia
  real y dispositivos físicos. No declarar todos los bugs universalmente cerrados.

## Web Alpha 2 publicada y verificada (16/10/2026)

- Código ca86d0016; Pages f9827a51ff92f6e8839103b3655dc700a2a44f27.
- https://matymeatboy.github.io/ReKHCom/ . 17/17 archivos públicos coinciden en
  SHA256 y tamaño, motores application/wasm, fuentes incluidas. WEB_PUBLICACION.json.
- Prerelease https://github.com/MatyMeatBoy/ReKHCom/releases/tag/web-alpha2 .
  ZIP 4.386.139 bytes, SHA256 1aafa9ab0552df868217a36749d1f3fa48237f80aa6f24a5269cf2f20777ee2f.
  GitHub confirma digest y prerelease=true. La latest estable sigue v1.8.3.
- Portada real Chrome: English por defecto, cambio Español y vuelta English
  verificados. docs/screenshots/web-player.jpg actualizado. Interfaz y lenguaje
  del cartucho son independientes. Alpha2 identifica la web en repo/README/About.
- Se publicaron fix resize/fullscreen, táctil centrado, HD fuentes originales,
  persistencia serializada/banco activo/reset protegido y caché por hash.
- No marcar todos los bugs o plataformas como resueltos: comprobar partida real
  Alpha2 y F11/Escape manual, guardado/bancos, US/JP, móviles y sesiones largas.
  La prueba real de fuente/SDL fue sin ROM; la prueba F11 automatizada fue de
  callbacks controlados porque requestFullscreen no se concedió al clic de tool.
  WEB_ALPHA2_TESTS.json y WEB_PENDIENTES.md conservan evidencia y pendientes.

## Web Alpha 2 lista para publicar (16/10/2026)

- EU/US/JP compilados sin avisos de firma. PPU 206/206; PC EU/US/JP compilan.
- Fuentes reales KHMenu/ComicHearts dibujadas en SDL/WebGL con prueba sin ROM:
  inglés, signos/acentos españoles y subtítulos. No confundir con partida real.
- Native SDL resize verificado 960x540 -> 769x433 -> 960x540. Fullscreen API no
  concedida al clic automatizado del navegador; casos F11/Escape/fullscreenchange
  probados en JS de producción con frontera nativa controlada, no como prueba
  manual de gameplay. El usuario debe confirmar ese ciclo en su partida.
- Pruebas Node de frontend PASS, táctil real C en 5 proporciones PASS,
  controles Android existentes PASS, audio 24/24 + 800k integrales PASS.
- WEB_ALPHA2_TESTS.json contiene el alcance. Falta despliegue/hash/release y
  actualizar evidencia pública. No afirmar que todos los navegadores/dispositivos
  o partidas largas estén verificados. WEB_PENDIENTES.md mantiene lo restante.

## Web alpha2 en desarrollo (16/10/2026)

- Usuario pidió: corregir F11/Escape y resolución, centrar controles táctiles,
  portar HD con fuentes de PC, idiomas web con inglés principal y release alpha.
- Implementado, aún en verificación: resize coalescido por ResizeObserver,
  fullscreenchange y window/visualViewport; export C encola tamaño, SDL aplica
  entre frames. Guardado serializado, banco activo al exportar, reset espera
  sync en Asyncify y no reinicia si falla. Banner persistente ante fallo IDBFS.
- HD Web ahora rasteriza con Canvas2D las mismas KHMenu/ComicHearts. La colocación
  se extrajo sin cambios a port_hdtext_layout.inc compartido con Windows.
  Caché 2 escalas/estilo, 1024 JP por font; sin duplicar imágenes JS. Falta prueba
  visual y regional. EXP sigue filtrado en PPU. Fallo de fuente vuelve a SD.
- UI web English por defecto, selector English/Español separado de idioma ROM.
- Táctil Web usa plano 16:9 centrado con margen; dibujo y hit tests usan la misma
  transformación. Android sin cambios de geometría.
- test_web_runtime.cjs PASS: tamaños fullscreen/Escape/F11, resize coalescido,
  idiomas, nombres por banco, sync serializado y fallo quota.
  web_touch_conformance.c PASS: centro y pulsaciones en 5 proporciones.
- Compilando EU/US/JP: build/web-alpha2-build.log (sesión75596). Emsdk6.0.12,
  SDL estático en D:/01Claude/ReKHCoM-build/web-sdl. No desplegar hasta compilar/probar.
- Próximo: prueba real de fuentes, regresión PC tras extraer layout, caché por
  hash de shell/motores/fuentes; publicar Pages y prerelease web-alpha2.

## GitHub Pages verificado (16/10/2026)

- URL pública operativa: https://matymeatboy.github.io/ReKHCom/ .
- Pages commit 4a7495e45451bf0c3ef8aa9d62c4d7e6ca65dd23, source 5c39af98b.
  Build GitHub: built. Los 12 archivos públicos coinciden por SHA256/tamaño;
  los tres Wasm se sirven como application/wasm. WEB_PUBLICACION.json.
- README/About enlazan Web; captura pública docs/screenshots/web-player.jpg.
  La portada conserva créditos, requisitos de ROM y foco en funciones del player.
- Prueba de gameplay sigue siendo EU local realizada por el usuario; la captura
  pública es del launcher. No confundir integridad/publicación con partida larga.
- Pendientes completos en WEB_PENDIENTES.md. HD Web NO está implementado; SD
  forzado evita diálogos vacíos. Próximo: guardar/recargar/exportar, US/JP reales,
  audio/pausa y móviles antes de asumir compatibilidad completa.

## Web publicado en rama y limpieza final (16/10/2026)

- Source d7f16c549 pusheado main/codex/v1.5-fixes. gh-pages e57d1e0ae con
  EU/US/JP finales, guardia SD por frame y manifiesto. Pages construyendo:
  https://matymeatboy.github.io/ReKHCom/ . About y README enlazan el juego.
- publish_web.py conserva bytes exactos con hash-object --no-filters para que
  autocrlf no invalide hashes en Windows. Regenerar manifiesto tras source commit.
- Pruebas finales: ABI Wasm PASS; PPU PC 206/206; PC EU/US/JP compilan;
  táctil PASS; stock EU/US/JP PASS; audio 24/24 + 800.000 integrales sin error.
- Limpieza: 134 duplicados de pruebas eliminados (1.155.677.212 bytes), 20
  temporales Windows >7 días (7.142.395 bytes), BMP generados convertidos a PNG
  con igualdad RGBA verificada (4.562.465.029 bytes ahorrados). Total 5.725.284.636
  bytes (~5,73 GB). C: ~9,7 GB libres. Inventario hasta profundidad 5 de los otros
  proyectos no encontró .tmp/.temp/.dmp antiguos; no se borró nada ambiguo.
- Manifiestos privados build/cleanup-20261016-{web-review,windows-temp,lossless-images}.json.
  Se conservaron fuentes, SDK, releases, ROMs/SRAM/states, trailer y cuatro imágenes
  del usuario en raíz. No limpiar toolchains activos para ganar espacio.

## Navegador: alpha y seguimiento (16/10/2026)

- Prioridad del usuario: publicar el juego real en GitHub Pages. EU probado por
  usuario en Chrome hasta diálogos iniciales; SD correcto. No afirmar test de
  combate Web, sesiones largas ni rendimiento móvil todavía.
- Emsdk 6.0.12 en D:/01Claude/ReKHCoM-build/toolchain/emsdk; SDL3 3.4.18 estático en
  D:/01Claude/ReKHCoM-build/web-sdl. build_all.py --platform web compila EU/US/JP.
- build_web.py transforma imports fijos LLVM a constantes; conserva ABI wasm32,
  relocaciones de funciones reales y Asyncify para bucles de juego/películas.
- test_web_abi.py PASS: constantes, hardware fijo, globales nativos, espera
  Asyncify y callback de retorno convertido. build/web-abi/report.json.
- Se corrigieron prototipos faltantes (Wasm atrapaba firmas void/int) y el export
  HEAPU8 del loader. La pantalla vacía al elegir HD era configuración activada
  después del arranque por Re:Chain: Web fuerza SD por frame, como Android.
- Port web: ROM local SHA-1, SRAM/config IndexedDB, importar/exportar .sav,
  controles teclado/mando/táctiles opcionales, pausa hidden y canvas sin DPR.
  Persistencia y mobile están implementados pero pendientes de pruebas completas.
- PC EU/US/JP recompilado; oracle PPU 206/206 PASS:
  build/ppu-conformance/20261016_130024_063894/report.json.
- Lista específica y prioridades futuras: [WEB_PENDIENTES.md](WEB_PENDIENTES.md).
  Consultarla antes de retomar HD, guardados, móviles y rendimiento.
- Publicación en preparación con publish_web.py: allowlist de archivos, hashes y
  árbol gh-pages aislado. No ROMs/SRAM/config. La estable PC sigue v1.8.3.
- Limpieza autorizada: 1.155.677.212 bytes de duplicados de transition-review;
  7.142.395 bytes de temporales Windows >7 días. Logs build/cleanup-20261016-*.
  Capturas BMP antiguas se convierten a PNG con verificación píxel a píxel;
  conservar fuentes, releases, ROMs, estados y partidas.

## Android: combate regional y nuevo objetivo Web (16/10/2026)

- Batalla Android EU/US/JP: los tres llegan a frame600, producen imagen real
  960x540 y salen en frame900 sin crash tras la corrección de stock. Informe
  build/android-runtime-battles/report.json; capturas eu/us/jp.png.
- US fallaba en IsKindPairThenNonSummonOfCategory (NULL+0x48, stockCount<3).
  Las dos funciones IsKindPairThen{NonSummonOfCategory,Summon} ahora usan también
  en PLATFORM_PC la comprobación count==3 que ya tenía EU. Se aplica a Android
  y Windows, conserva combos válidos y evita depender de SEH en US/JP.
  test_stock_count.py pasa EU/US/JP con stocks incompletos y combos válidos.
- Bibliotecas Android exportan solo SDL_main (exports.map). Dynsym EU x86
  75.383 -> 168 entradas; las restantes son imports. Globales/secciones siguen
  resolviéndose internamente y el juego arranca. Símbolos de debug conservados
  en build/android-native*, los staged .so usan --strip-unneeded.
- build_android.py --smoke crea otra app com.rekhcom.android.smoke y artefacto
  marcado test_only, con batalla/captura/salida automáticas via SDL_ENV. No subir
  ese APK. El APK normal no contiene SDL_ENV. test_android_runtime.py exige
  serial explícito y usa solo el almacenamiento de la app de pruebas.
- Los binarios empaquetados se cotejan por SHA con los staged, y Gradle clean
  evita huecos de empaquetado incremental. NDK fijado también en Gradle.
- APK ARM actual (tras stock/export/strip/launcher) compilada, NO publicada:
  SHA256 898c19a5bd862d27844ff6d0d80a49f7ceaa7aa5d91afbdd23dc694850260a0f.
  ARM físico no probado. x86 smoke probado; normal debe reconstruirse también.
- Usuario aclaró HD: Ver barajas, pantalla de añadir/quitar naipes. Capturando
  ahora AddGrid / AddValueSelect, no solo el popup de comandos.
- NUEVO pedido autorizado: port navegador con ROM desde Archivos, hospedado
  en GitHub Pages. Comenzó análisis SDL3/Emscripten/Asyncify y memoria GBA.
  Pages aún no existe (API404); no publicar launcher vacío como port jugable.
  Resolver ABI/símbolos de Wasm y bucle antes de desplegar; misma prioridad de
  no regresiones PC/Android. Documentar en reparto a cada avance.

## Regresión regional y opciones táctiles (16/10/2026)

- EU/US/JP: 18/18 imágenes SDL idénticas a los EXEs publicados en v1.8.3:
  campo, combate y Barajas en 1920x1080 y 2560x720. Informe
  build/v19-regions-report.json; test_v19_regions.py extrae solo EXEs de referencia.
- Comparación de composición: 60/60 frames exactos, menús Deck/Mapinspect/Estado
  y puertas, 16:9, 32:9 y resize en caliente. build/v19-composition-regression.json.
- Escalas x2/x3/x4/x5/x6: todos los hashes de imagen coinciden; x1 ya repetido.
- Encontrado/corregido Android: el botón … mandaba bits GBA pero nunca abría
  PortUi (solo escuchaba eventos SDL). El touch ahora abre esa UI y navega por
  la misma ruta teclado; pruebas de multitoque/cancel/foco/diagonales/UI pasan.
  También comprobado en emulador: abre opciones, cambia zoom x3 -> x2, B cierra,
  al reabrir conserva x2 y config zoom=4. Capturas build/android-options-{fixed,
  nav,reopen}.png. No confundir estos controles con el prototipo HTML.
- Launcher Android ofrece solo zooms que caben en la pantalla apaisada; x4 sigue
  por defecto donde cabe. En 540p máximo x3 (160 líneas mínimas del juego), antes
  el selector ofrecía x4 aunque el motor terminaba limitándolo silenciosamente.
- Reconstruir APK ARM y x86 tras este checkpoint (launcher/ruta UI cambiados).
  Alpha y v1.9 todavía sin publicar; v1.8.3 permanece estable. Falta comprobar
  combate Android y todo lo indicado en la cabecera anterior.

## x1: segunda optimización verificada (16/10/2026)

- Nueva ruta de composición para filas de mundo con fondos opacos, sin OBJ,
  ventanas ni efectos de color aplicables. Mantiene máscaras de prioridad HD/HUD;
  efectos, mensajes acoplados y sprites conservan la ruta completa.
- Oracle independiente ampliado de 108 a **206/206**: mismos casos con mundo
  ancho activado, recogida de prioridades y combinaciones de ventanas/mezcla.
  Informe build/ppu-conformance/20261016_111841_642473/report.json.
- Campo x1, 1920x1080, SDL dummy: primer par PPU 51,34 -> 39,04 ms; repetición
  48,14 -> 39,34 ms (**18,3% menos en el par repetido**). Presentación 21,36 vs
  21,48 ms prácticamente igual; hashes exactos de la imagen antes/después.
  Batalla x1 28,26 -> 27,16 ms en este par; no sumar porcentajes entre pruebas.
  Informes build/zoom-benchmark/v19-opaque-{before,after}[-repeat]/report.json.
  Son tiempos CPU del render de software, no FPS de un GPU real ni promesa 60fps.
- En curso comparación x2/x3/x4/x5/x6. Aún falta matriz EU/US/JP, menús/puertas,
  ultrawide y reconstruir Android con la nueva optimización antes de publicar.
- Android x86: SRAM EU carga, Sora se mueve, menú normal abre, pausa/reanudación
  mantiene PID; solo 2 ticks CPU durante 3s en background. Sin crashes de runtime
  en la sesión corregida. build/android-runtime-smoke.json. ARM físico pendiente.
- Barajas HD: con la SRAM real y entrada natural al juego, ni foto fija ni
  frames 2220-2241 muestran con claridad el solapamiento reportado. Evidencia
  build/transition-review/deck-{normal-save-hd0,normal-save-hd1,submenu-slide-hd}.
  No declarar corregido por esta ausencia de reproducción. Continúa revisión.

## Compilación tras corregir SHN_ABS

ARMv7 EU/US/JP y x86 EU/US/JP compilan; el verificador ELF pasa en los seis.
APK ARM actual: baf793f4ea7a59a0a55c27eb3e63be36415f982a58bfbfce1777cf35e4c22ace.
APK x86 actual: 17e8302522d3ebdae8513090004bb90f541e72583dd0cdcfccaf6693b6780b5d.
Windows EU incremental pasa. Audio: 24/24 bordes PCM y 800.000 integrales sin
error. Fullscreen comprobado en emulador; ROM y SRAM EU cargan sin modificar
los originales del PC. Continúa la prueba de cargar partida, pausa y campo.

## Retoma: Android en ejecución (16/10/2026)

- APK x86 de prueba instalada en emulador oficial API24. Launcher SAF importó
  ROM EU desde Archivos; el juego alcanza selección de idioma y título, y los
  botones táctiles respondieron. No confundir con validación de ARM en móvil.
- Primer arranque reprodujo SIGSEGV en SoundMainBTM: Android rebasa SHN_ABS,
  incluido gNumMusicPlayers=26; el bucle sale de tabla. -Bsymbolic resuelve
  localmente estas referencias; tras corregirlo el título arranca. Verificador
  ELF en build_android.py rechaza futuras relocaciones a símbolos absolutos.
- AVD aislado en D:/01Claude/ReKHCoM-build/android-avd (C: sin espacio suficiente).
  Scripts/logs locales build/start_android_emulator.py, build/android_ui.py,
  build/android-{first,second}-launch.log y capturas android-title.png.
- Liberados 1.073.755.621 bytes de ZIPs ya extraídos y con hash oficial
  verificado. Manifiesto build/android-archive-cleanup.json; ROM/saves intactos.
- En prueba: fullscreen Android, recreación de iconos tras reinicio del renderer,
  cierre único del archivo SRAM en errores (antes podía cerrarlo dos veces).
- Pendientes PC: Barajas HD, regresión completa, rendimiento x1, cerrar v1.9.
  Pendientes Android: campo/combate, lifecycle/SRAM, ARM real y alpha pública.
  Stable v1.8.3 intacta. No publicar la APK como validada aún.

# v1.9 — trabajo activo, NO publicada (16/10/2026)

## Pedido vigente

Usuario autoriza todos los pendientes para v1.9: optimizar PC en todas sus
variantes, usar x1 de benchmark, conservar corrección visual/audio, resolver
pendiente HD de Barajas. También construir y probar el port Android (backend
ARM, launcher con selector de ROM y controles táctiles), publicar alpha solo
cuando sea jugable. Autoriza SDK/NDK y sus licencias, así como limpiar temporales.
No sustituir la stable v1.8.3 ni su ZIP mientras v1.9 no esté verificada.

## Cambios implementados y primera validación

- `port_ppu.c`: buffers de canvas/oclusiones crecen hasta el tamaño necesario,
  conservando capacidad en los cambios menú/mundo; cero en la parte nueva.
  Realloc al cambiar resolución; no se cambian dimensiones ni escalas visibles.
- `port_main.c`: presentación al tamaño activo, HUD asignado solo al usarse.
- `DrawStraightAffine`: camino pa=256, pc=0, sin mosaico horizontal. Reutiliza
  únicamente las coordenadas X; clave incluye scroll, BG control, recorte,
  geometría y bandas de arena. Lee VRAM y paleta de nuevo en cada scanline.
  Conserva el camino general para rotación, escala o mosaico.
- Benchmark: `python port/tools/benchmark_zoom.py --label <nombre> --frames 480`.
  Subprocesos aislados, SDL dummy, 1080p, x1/x2/x4, campo y batalla, frame479.
  `build/zoom-benchmark/v183-before/report.json` y `v19-first/report.json`:
  las **seis imágenes finales coinciden SHA-256**. No mide GPU real ni una
  partida completa. Registra capacidad privada y working set por separado.
- Repetición x1 para evitar atribuir diferencias de carga/frecuencia a la mejora:
  `v183-repeat/report.json` frente a `v19-repeat/report.json`.
  Campo PPU 30,09 → 29,35 ms (~2,5%, no mejora sustancial aún).
  Batalla PPU 24,05 → 17,87 ms (~25,7% menos tiempo).
  Commit privado pico en campo x1 361,60 → 322,57 MiB; en x4 la primera
  comparación 356,30 → 284,81 MiB. **No afirmar que baja el RSS:** sus picos
  son similares o ligeramente mayores. Los buffers retienen la capacidad máxima
  alcanzada durante esa ejecución; no reducen capacidad al volver a x4.
- Oracle PPU: 104/107, sin fallos nuevos; los tres históricos son
  semantic_hd_hud_cover_bg/cover_obj/window. Todos los píxeles hardware pasan.
  `build/ppu-conformance/20261016_045819_092143/report.json`.

## Pendientes antes de publicar PC

- [ ] Optimizar más campo/x1, midiendo antes/después con carga comparable.
- [ ] Probar explícitamente crecimiento y cambios de resolución/zoom, HUD, estados,
      menús/puertas/cinemáticas, HD on/off y ultrawide; comparar con v1.8.3.
- [ ] Resolver HD sobre submenú de Barajas; revisar los tres fallos semánticos.
- [ ] Comprobar audio, compilar EU/US/JP y actualizar versión a v1.9 solo al cerrar.
- [ ] Empaquetar, revisar SHA/privacidad, publicar stable y preparar copia local.

## Android

Estudio y bloqueos: `port/android/README.md`. Prototipo `preview.html`, iconos
propios `assets/roll.svg` y `stock.svg`. SVG parseados y rasterizados con sharp;
JS del prototipo pasa análisis sintáctico. El visor del navegador bloqueó file://,
por lo que no se verificó interacción táctil real; no eludir esa restricción.
Todavía **no hay APK ni backend ARM**. No anunciarla como disponible.

Usuario aceptó expresamente licencias SDK/NDK e instalación en toolchain.
Instalador local `build/setup_android_tools.py` (sesión shell 98801 al escribir
este checkpoint): descarga SDK commandline-tools latest, NDK r30
30.0.16248370 y Temurin JDK17 desde Google/Adoptium; verifica SHA oficiales.
Instalación en `../toolchain/android/`, manifiesto de URLs/hashes en
`../toolchain/android/downloads.json`. Confirmar finalización antes de utilizar.
Faltan SDK platform/build-tools, SDL Android, Gradle/CMake y proyecto nativo.
La compilación ARMv7 necesita sustituir PE/Win32, GDI, ensamblador COFF y el
manejador x86 de fallos BIOS; arm64 además requiere resolver punteros de 32 bits.

## Limpieza y preservación

Se eliminaron 1.377 capturas `out_*.bmp` / `shot_*.bmp` de ocho carpetas de
review dentro de `build`, todas verificadas bajo el workspace. 6,50 GiB libres
recuperados, unos 12,68 GiB disponibles antes de instalar Android. Manifiesto
`build/cleanup-20261016.json`. Se conservan PNGs de evidencia, ejecutables,
informes, ROMs, partidas/estados, herramientas, tráiler final y sus fuentes.
No tocar los cuatro archivos gráficos no trackeados del usuario en la raíz.

## Integración / continuación

Rama local `claude/windows-5`; subir explícitamente a main y codex/v1.5-fixes.
Última stable: tag v1.8.3, release 414677687, commit juego cfc5b6392,
documentación d462f08cc. Android/diseño/build_all subido en 6d5cdbaca.
No usar `git add .` por los gráficos privados no trackeados.


## Actualización Android y HD (último checkpoint)

- Herramientas instaladas y verificadas. SDK CLI actual usa android.exe sdk
  install por paquete; sdkmanager.bat acepta argumentos viejos pero no instaló
  todas las plataformas. Ya existen platform35/build-tools35.0.0/CMake3.22.1.
- Android CMake pasa; librekhcom_eu.so/us.so/jp.so compilan. SDL3 y núcleos
  ARMv7 ABI21; las secciones khdata/khbss y SDL_main se exportan correctamente.
- Primera APK debug compilada (NO PUBLICADA), manifiesto runtime_tested=false.
  build/releases/ReKHCoM-v1.9-android-alpha1-armeabi-v7a-debug.apk.
  SHA inicial a4e06be306f24e3a49bc744cf7ee3473f416645ca7e6d043ab564159005d3557;
  se deberá reconstruir porque los últimos cambios de lifecycle son posteriores.
- Launcher Java real: SAF ROM con SHA, importar/exportar SRAM, configuración,
  idioma y tres núcleos cargados por separado en :game. Multitoque SDL real,
  iconos embebidos desde SVG; pausa en background implementada sin probar.
- build_android.py y build_all.py --platform android automatizan toda la matriz.
- No hay dispositivo adb conectado. Instalando emulator e imagen API24 x86 en
  sesión 13007; el emulador moderno en host x86 no soporta ARMv7 directamente.
  Próximo paso: permitir ABI x86 (también 32bit) como build de prueba aparte,
  ejecutar allí y distinguir esa evidencia de probar ARM en un móvil.
- Gradle primer intento falló por no definir repositorios de dependencias de
  app (aapt2); corregido dependencyResolutionManagement, siguiente build pasó.
- Error inicial ARM por malloc.h de juego sombreando bionic: -iquote en CMake.
- Oclusión HD HUD corregida: se usa posición OAM original como en RenderHud,
  conservando la excepción del mensaje propio. Oracle ahora **108/108**; agregado
  semantic_hd_hud_message para no volver a ocultar diálogos propios.
  build/ppu-conformance/20261016_101342_964485/report.json.
- Intentos adicionales de filas uniformes / caché de map entries descartados:
  no mostraron mejora medible en campo; no están en el código. Se conserva solo
  la optimización affine y de capacidad verificada en 3fe40fe29.
- Falta revisar HD sobre submenú real de Barajas (estilo0 y BG estilo2), y toda
  la matriz Windows/Android antes de publicar v1.9/alpha. v1.8.3 no modificada.
