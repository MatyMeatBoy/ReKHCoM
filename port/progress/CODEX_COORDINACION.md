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

## Riku: persistencia, carga y textura corregidas (17/10/2026)

- Detalle y reproduccion: [RIKU_BUGS_V19.md](RIKU_BUGS_V19.md).
- Launcher conserva el truco; carga muestra Riku solo con truco ON o Sora completado.
  Titulo actualiza acceso al cerrar opciones. No fabrica Sora completado.
- Copia del save del usuario: carga y guardado nativo de Riku comprobados;
  dos Sora y otra ranura Riku intactos. Archivo original no modificado.
- Fondo inicial: un solo circulo reconstruido con indices de paleta/tramado
  nativos; pantalla GBA exacta, sin negro ni segundo circulo. PPU206 + tests
  dedicados de panel/paleta/fade y EXP768 PASS. EU/US/JP PC compilan.
- Unicamente desarrollo/local v1.9. No publicar ni crear otras versiones aun;
  v1.8.3 sigue siendo la unica release publica. Otros cheats y zoom reportado
  siguen pendientes; no incluir menus relativos experimentales en stable.

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

## Controles Web y archivos moviles: rama aislada (16/10/2026)

- `fix/web-mobile-files-controls`: teclado SDL limitado a #canvas, limpiar teclas,
  touch y acciones pendientes al perder foco/cancelar/rotar; selectores sin filtro
  de extension para Archivos iOS; exportar con Descargar .sav y Compartir/Archivos
  por clic nuevo. Fallback de viewport para Safari sin fullscreen de elemento.
- Node + touch + input/SDL virtualpad PASS; Chrome/Wasm real sin ROM observa
  las 13 teclas (8191), libera held=0 y escribir fuera no aumenta presses=13.
- EU/US/JP Web compilan. No publicar aun: pendiente gameplay con ROM elegida
  manualmente y dispositivos fisicos. Permiso de extension sigue bloqueando
  seleccion automatica de archivos; no eludir con un servidor de ROMs.
- Ver WEB_CONTROLS_REVIEW.md. No confundir prueba de backend sin ROM con partida.
- NUEVA POLITICA: mejoras grandes de menus SOLO feature/relative-menu-scale;
  usuario confirma expresamente que no entran en stable. No push a main ni Pages
  de esa funcionalidad. Publicacion de correcciones independientes requiere
  registrar su alcance y validacion, sin arrastrar el experimento de menus.

## Android Alpha 2 publicada; objetivo RG34XX confirmado (17/10/2026)

- Release prerelease: https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/android-alpha2
- Tag en 4610b08ef8567d38b31c0173694724d63d026f18, no main. Stable v1.8.3 intacta.
- APK final ARMv7 debug SHA256 34b1e4a1879ffdf4bb91133a43e1eb61fc098222b7d5b5efb2736db7396cdeee.
- .sav/.srm, bancos 1..5, backup y comprobaciones de formato implementados.
- 4:3 / GBA 3:2 con zoom implementados. Resolucion 720x480 x1/x2/x3 probada en PC.
- Android EU/US/JP combate en emulador x86 PASS; no equivale a ARM fisico/SAF.
- Objetivo Linux confirmado RG34XX: ARM64, 720x480, 3:2; Steam Deck x86-64 aparte.
- NO hay build Linux/ARM64 todavia. Cheats/editor de cartas y menus relativos pendientes.
- Reparto detallado: [PLATFORM_ALPHA_PLAN.md](PLATFORM_ALPHA_PLAN.md).

## Android, Linux / Steam Deck y nuevos pedidos (17/10/2026)

Reparto vigente: [PLATFORM_ALPHA_PLAN.md](PLATFORM_ALPHA_PLAN.md).
Android primero; Linux y Steam Deck despues. Todo salvo Windows marcado alpha.
Importacion .sav/.srm por contenido, region y banco; cheats OFF, modo Riku,
editor de cartas y 4:3 con zoom/HD pendientes. Menus relativos fuera de stable.

## Quiver enviado por el usuario; nombre oficial ReKHCoM (16/10/2026)

- Usuario confirma que ya envio la propuesta a Quiver. No reenviar ni crear PR
  duplicado. Aceptacion/publicacion aun sin verificar; falta URL o recibo.
- Repo renombrado via GitHub API de ReKHCom a ReKHCoM (ultima M mayuscula).
  Origin y enlaces activos actualizados; URLs de historia abajo son originales.
- Canonico: https://github.com/MatyMeatBoy/ReKHCoM
- Web: https://matymeatboy.github.io/ReKHCoM/
- Icono: https://raw.githubusercontent.com/MatyMeatBoy/ReKHCoM/main/docs/quiver/icon.png
- GitHub redirige referencias al repo anterior. Verificar Pages tras despliegue.

## Quiver Launcher: kit listo, envio pendiente de login (16/10/2026)

- Pedido del usuario: proponer ReKHCoM, preparar banner/assets y PR.
- Cambio de canal oficial verificado: quiver-community-app-catalog archivado el
  09/10; no acepta PR/issues. Los envios van a https://quiverlauncher.com/submit.
- Formulario requiere login. Pestana Chrome abierta; usuario avisado por pregunta
  asincrona. NO se envio propuesta, NO existe PR y NO afirmar aceptacion.
- docs/quiver/: 5 PNG + SVG editables, ficha inglesa, submission-draft.json,
  my-app-list.json. Banner 1920x620, capsule 600x900, hero 1920x1080,
  logo transparente 1200x240, icon 512x512. Dimensiones comunes propuestas;
  falta contrastar requisitos del formulario tras login.
- Windows v1.8.3 unico stable: SHA256 y contenido ZIP verificados contra GitHub.
  Filtro windows-x86.zip; entrada ReKHCoM.exe; preservar bin/assets/DLLs.
- API publica no encuentra ReKHCoM. Propiedad preferredExecutables windows
  observada en catalogo actual; verificar disponibilidad en formulario.
- Siguiente: login -> revisar campos, metadatos IA honestos segun definiciones
  oficiales -> enviar -> guardar recibo -> probar instalacion/arranque Quiver.
  No abrir PR al catalogo archivado ni issue de reemplazo que ignore su canal.

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

# Trabajo activo — Android y rendimiento PC v1.9 (16/10/2026)

- [x] v1.8.3 única stable publicada, tags previos conservados, README/capturas actuales.
- [x] Estudio Android, bloqueos nativos y ruta SDL/NDK/launcher SAF: ../android/README.md.
- [x] Diseño táctil GBA, iconos vectoriales propios de rodar/apilar y prototipo HTML.
- [x] build_all.py configura/compila EU/US/JP juntos; compilación incremental verificada.
- [x] Limpieza autorizada: 1.377 BMP generados, 6,50 GiB recuperados. PNG de evidencia,
  informes, ROMs, partidas/estados, ejecutables y tráiler final conservados.
- [x] Android: SDK/NDK/JDK instalados; backend ARMv7 EU/US/JP compila y APK debug generada.
- [~] Android: pruebas de ejecución, lifecycle y controles; APK aún NO publicada.
- [x] Benchmark x1/x2/x4 y primera optimización PC: seis imágenes idénticas.
- [x] Buffers por capacidad activa y caché de coordenadas affine; oracle sin regresiones.
- [~] PC v1.9: campo/x1, HD en submenú de Barajas, matriz de regresión y release.
- [x] Tres fallos semánticos HUD corregidos; oracle ampliado 108/108, mensaje propio visible.

Checkpoint detallado y comandos: RELEASE_V19.md. Batalla x1 repetida: PPU
24,05 → 17,87 ms; campo 30,09 → 29,35 ms. No prometer aún 60fps ni mejora total.

El prototipo táctil aún no está conectado al juego. No anunciar Android disponible.
Detalle limpieza: build/cleanup-20261016.json. No se borraron archivos del usuario.

---

# Relevo activo — v1.8.3 (16/10/2026)

Pedido del usuario a Codex: resolver el guante estirado de Naipes mapa.
Claude dejó sin commitear la extensión de PortMenuBackgroundOnly a
Mapinspect y Deck; Codex verifica e integra ese cambio.

- [x] Reproducir la franja blanca en la ventana final, frame219 a 1536x864.
- [x] Usar los fondos de ambos paneles; conservar el guante original central.
- [x] Antes/después: entrada/salida de Naipes mapa y Ver barajas, tres
  resoluciones y zoom x3/x4/x6; regresión de Estado.
- [x] Compilación EU/US/JP.
- [x] ZIP verificado; copia local v1.8.3 con partidas normales conservadas.
- [x] v1.8.3 publicada como única stable; release v1.8.2 retirada, tag conservado.
- [x] README actualizado para jugadores; 23 capturas de v1.8.3 revisadas.

El texto HD por encima del submenú de Ver barajas sigue siendo otro pendiente.

---

# Relevo activo — v1.8.2 (Codex, 16/10/2026)

El usuario autoriza a Codex a integrar y republicar tras agotarse Claude.
Este bloque sustituye las exclusividades históricas de abajo.

- [x] A: fondo actual en transiciones e idioma sin emblemas repetidos.
- [x] B: histograma exacto compartido; margen derecho del título corregido.
- [x] C: blank antes de reutilizar gráficos; fixture real antes/después.
- [x] Matriz: 270 frames presentados en 1536x864/1920x1080; EU/US/JP compilan.
- [x] v1.8.2 republicada: ZIP/SHA verificados, tag anterior conservado y copia local anterior.
- [x] Tráiler v4 local terminado y verificado; scripts/continuidad en TRAILER_V4_ENTREGA.md.
- [ ] Futuro: HD sobre el submenú de barajas, aplazado en el último reparto.

Detalles: RELEASE_V182.md. Los intentos previos de A-C quedan sustituidos.

---

# CODEX: TAREA ACTUAL — 3 bugs para v1.8.2 (16/10/2026, Claude)

Haz SOLO estos 3 bugs, en este orden. No toques zoom, márgenes de menús,
HD HUD, HD-2D, audio ni tráiler. Commits locales en `khcom-claude`
(rama claude/windows-5); Claude integra, publica v1.8.2 y la sube encima
de la release v1.8.2 actual. Al terminar cada bug, anota aquí resultado y
evidencia. El 4º (números HD sobre el submenú de Ver barajas) queda para
el futuro: NO lo hagas ahora.

Verificación obligatoria: lo que ve el jugador es la ventana. KHCOM_SHOTS
guarda el lienzo/ventana del arnés, pero en escenas GBA-size los márgenes
pueden venir del lienzo del PPU (fuera de la columna GBA) y no del
presentador; confírmalo con KHCOM_RECORD=desde-hasta:ruta.mov (lee la
ventana) o comparando con capturas del usuario. Ventana de prueba 1536x864
(la del usuario) y 1920x1080. No publiques nada sin ver el frame exacto.

## Bug A — Transición tras elegir idioma (parpadeo + fondo antiguo)
Reproducir: arnés sin KHCOM_MODE, lang=None, KHCOM_KEYS=200-203:A,
KHCOM_SHOTS=236..260 (cada frame), KHCOM_WINDOW=1536x864.
- Frame 251 (primer frame de mode_copyright1, PortSideStyle COLOR=1):
  centro negro y márgenes BLANCOS -> parpadeo. Frames 250 y 252: todo
  negro / todo blanco (correctos).
- Frames 246-247 (fundido de mode_lang, estilo TEXTURE=2): los márgenes
  repiten los iconos de Sora/corazones de la fila inferior ("fondo antiguo"
  que el usuario ve 2 frames). Causa probable: al oscurecerse, las bandas
  con emblema pasan el umbral "clean" (hi-lo<=64, croma<=40) y se copian.
- KHCOM_DBGSIDE=1 (ya en el código) loguea por frame: style, fit, wide,
  src. En 251: style 1, fit 0, wide 0, src 72,28 240x160 (lienzo 384x216).
- Intento de Claude ya en el código, en port_main.c (presentador):
  UniformPicture() pinta los márgenes con el color plano cuando la imagen
  GBA es uniforme, y SideBar TEXTURE usa umbrales relativos al contraste.
  NO cambió las capturas -> los márgenes vistos salen de otra parte
  (lienzo PPU fuera de 72..312 o del primer frame del modo nuevo con
  estilo/datos del anterior). Encuentra quién pinta esos píxeles y arréglalo
  ahí.
- Objetivo: fundido suave, sin ningún frame con márgenes de distinto color
  que el centro ni iconos repetidos.

## Bug B — Margen derecho oscuro al entrar a Cargar desde el título
Captura del usuario: tras elegir CARGAR en el título, durante el fundido a
negro el margen derecho queda casi negro mientras el izquierdo sigue gris
(igual que el centro). Ventana 1536x864, partida existente (save=True).
Reproducir: KHCOM_KEYS con A 200..800 cada 60, START 1120, luego DOWN y A
sobre CARGAR cuando aparezca el menú (~frame 1400-1430) y capturar cada
frame del fundido siguiente. Probablemente el mismo mecanismo que el Bug A
(margen que no sigue el brillo del centro). Objetivo: ambos márgenes con
el mismo color/brillo que el borde de la imagen en cada frame.

## Bug C — Destello del cuarto anterior al cambiar de sala
Usuario: puerta arriba-derecha (isométrico) en su partida; state en
ReKHCoM-v1.8.1/ReKHCoM-v1.8.1-windows-x86/states/slot1.khst (SOLO carga
con build/v181/khcom.exe = exe de la release v1.8.1, por BuildId).
Con ese exe y KHCOM_LOADSTATE: el fundido a blanco termina ~frame 36-38;
frame 39 muestra pilares/objetos del castillo en BLANCO sobre NEGRO.
KHCOM_PPULOG en ese frame: "latch 0110" (BG3 pierde el latch del mundo un
frame antes que BG1/BG2), siguiente frame dispcnt 0040 (BGs apagados).
Intento de Claude (en v1.8.2, NO verificado): gracia de 1 frame en
PortPpuBeginFrame que mantiene el latch del BG que lo pierde. Verificar
con un state nuevo: el usuario puede guardar uno con v1.8.2 frente a la
puerta, o reproducir saliendo del castillo por una puerta con el arnés.
Objetivo: ningún frame con siluetas del cuarto anterior; el fundido pasa
de blanco a la sala nueva sin destellos.

## Después de los bugs: tráiler v4
Sigue `port/progress/TRAILER.md` (pasos, scripts en `Trailer/tools/` y
mejoras pedidas). Solo cuando los bugs A-C estén publicados.


# Abiertos tras v1.8.2 — 16/10/2026 (Claude). El usuario confirma que NO están corregidos en v1.8.2.

1. Ver barajas: al abrir el submenú, los números/textos HD siguen encima del
   menú. Intento v1.8.2: recorte por columnas en DrawSmallNumbers (solo
   estilo 2, solo BG); pasó en el arnés a 1536x864 pero no en el juego real.
   Revisar también glifos de estilo 0 y el orden capa HD vs. sprites del menú.
2. Cambio de sala (puerta arriba-derecha de la partida slot 1, v1.8.1
   states/slot1.khst): 1 frame con siluetas blancas del cuarto anterior.
   Causa vista con KHCOM_PPULOG: frame con latch 0110 (BG3 pierde el latch
   antes que BG1/BG2). Intento v1.8.2: gracia de 1 frame del latch; NO
   verificado (el state solo carga con el exe v1.8.1: BuildId).
3. Título -> Cargar: el margen derecho se oscurece durante el fundido.
   No reproducido en el arnés.
4. Arranque: justo después de elegir idioma, 1 frame con los márgenes sin
   pintar (negros) antes de la pantalla siguiente. Probable cambio de modo
   (mode_lang -> copyright) en el que PortSideStyle/margen aún no tiene datos;
   capturar frame a frame desde ~frame 250 con KHCOM_SHOTS.

   Hallazgo (Claude, arnés 1536x864, A en frame 200): fundido de mode_lang
   236-249 con márgenes de textura que no siguen el brillo del centro y que
   repiten los iconos de Sora de abajo (frames 246-247); frame 251 (primer
   frame de copyright1) con centro negro y margen derecho BLANCO. El usuario
   pide además que esa transición sea suave. Prioridad para v1.8.2 junto con
   2 y 3; el punto 1 (números HD) puede quedar para el futuro.

   Estado del intento (Claude, sin publicar): en el presentador (port_main.c)
   UniformPicture rellena los márgenes con el color plano al final de un
   fundido, y el estilo TEXTURE usa umbrales relativos al contraste. NO cambia
   las capturas del arnés: KHCOM_DBGSIDE=1 muestra frame 251 = style 1
   (copyright1), fit 0, src 72,28 240x160, o sea el lienzo es 384x216 y los
   márgenes de las capturas salen del lienzo del PPU (fuera de 72..312), no
   de SideBar. Siguiente paso: ver qué pinta el PPU fuera de la columna GBA en
   escenas no-wide (RenderLine con sInWorld/sLeft) en el primer frame de un
   modo, y verificar con KHCOM_RECORD (lectura de la ventana), no KHCOM_SHOTS.

Regla: quien trabaje (Claude o Codex) documenta aquí los pendientes al
avanzar; si a uno se le acaban los tokens, el otro continúa desde esta lista.
Solo Claude publica releases; Codex deja commits locales para integrar.

# Pendientes tras v1.8.1 — 16/10/2026 (Claude). Solo Claude integra y publica.

**Claude (por solucionar)**
- Zumbido de audio (hecho en v1.8.1): rampa de volumen por canal en el
  mezclador m4a. Queda revisar los pasos por frame del PSG/CGB (canales 1-4)
  en combate: el salto en el límite de frame bajó de 3.2x a 2.7x, no a 1x.
- Verificar el ritmo de frames en un PC real con la partida guardada del
  usuario (combate de Ciudad de Paso): en el arnés el trabajo por frame es
  ~20 ms por el renderer software; comprobar que no haya cortes de audio.
- Tráiler v4 (pausado por el usuario).

**Codex**
- Rendimiento del motor (ver reparto v1.7 abajo): 60 fps en x3+ a 1080p.

# Actualización — v1.8 (16/10/2026)

Pedido final del usuario: publicar v1.8 (la propuesta v1.7.2 no se publicó).
HD solo donde ya correspondía a diálogos/subtítulos/menús; números EXP originales
siempre y en su posición anterior. Selector sin filas de enteras/escalado,
anotaciones x6→íntegra x1 con aviso y x3→íntegra x2. x4 actual predeterminado.
Incluye bordes cinematográficos de las ranuras 1/2/3. Ver RELEASE_V18.md.

---

# Coordinación vigente — 16/10/2026

Claude sin créditos. El usuario autoriza a Codex a integrar el último trabajo,
corregir el audio (ranura 1 de v1.6.7), optimizar audio/gráficos, publicar una
release unificada y retirar releases anteriores conservando los tags.
Este pedido reemplaza temporalmente la exclusividad de integración de abajo.

Codex trabaja en khcom-claude; integrados los tres commits de rendimiento de
codex/ppu-performance. Ver AUDIO_BUZZ_V171.md y RELEASE_V171.md para evidencia.
No tocar el trabajo HUD antiguo sin commitear de khcom ni los guardados reales.

---

# Reparto v1.7 — 16/10/2026 (Claude). Solo Claude integra y publica.

Codex: trabaja en `khcom`, commits locales, sin push ni releases. Claude
integra tus commits en `khcom-claude` (rama claude/windows-5) y publica.
No tocar los archivos de la columna de Claude mientras esté este bloque.

**Codex — rendimiento del motor (prioridad)**
- Objetivo: 60 fps con el lienzo grande (1920x1080 en x1; hoy PPU ~86 ms,
  960x540 ~21 ms; ~40 ns/píxel). Medir con KHCOM_PROFILE=1 (log
  "profile: canvas WxH ppu X ms present Y ms" cada 240 frames).
- Ideas: RenderLine/ComposeLine por franjas en hilos (SDL_Thread, sin
  estado compartido por línea), saltar BGs/columnas transparentes, tablas
  de tiles precalculadas, evitar trabajo por píxel fuera de sLeft/sRight.
- Archivos: port/src/port_ppu.c (solo el render por línea; no cambiar
  semántica de márgenes/headers/HUD), port/tools/ppu_conformance.c.
- Aceptación: capturas idénticas píxel a píxel antes/después (batería de
  menús, campo, combate a 1920x1080 y 5120x1440) y tiempos del perfil.

**Claude — interfaz y escalado**
- Zoom x1..x6 (vista en múltiplos de la GBA; x1 Original, x2 Mejorado por
  defecto) con opción Escala entera ON/OFF.
- Parpadeo de las extensiones laterales durante el slide de submenús
  (Mapamundi, Estado...; el Diario no lo tiene).
- Relación de aspecto: opción para forzar ultrawide (21:9 / 32:9).
- Resolución completa: navegar y aplicar con A.
- Opciones por encima del HUD (hecho).
- Archivos: port_main.c, port_ui.c, port_config.c, decompui.cpp, src/*.

# Reparto tras v1.6.7 — 16/10/2026 (Claude)

Publicada v1.6.7 (única release pública). Zoom = escala de píxel entera
(gPortConfig.zoom: 0 Mejorado = x4 a 1080p; N = xN), buffers del lienzo en el
heap (PORT_MAX 2560x1440). Arnés: KHCOM_RESIZE=frame:WxH, KHCOM_PROFILE=1.

**Codex**
1. Rendimiento del PPU por software (~40 ns/píxel): x1/x2 a 1080p no llegan a
   60 fps (86 y 21 ms). Optimizar ComposeLine/DrawTextBg o hilos por franjas.
2. Guardar partida: el usuario vio paneles cortados en v1.6.5/6 (captura con el
   castillo de fondo). No se reproduce en v1.6.7 a 1920x1080 ni 1536x864 con
   zoom 0/1/2/5; confirmar con su partida real y otras salas.
3. HD HUD y HD-2D siguen ocultos y forzados a 0 (KHCOM_HD_HUD_DEV / KHCOM_HD2D_DEV).

**Claude**
1. Tráiler v4 (pausado por el usuario): intro de consola en el Castillo del
   Olvido, salas encadenadas a media escalera, post-créditos Sincorazón ya hecho
   (Trailer/clips/16_postcreditos_android.mov). Regrabar clips con v1.6.7.
2. Revisión de bugs que reporte el usuario.

# HD HUD: pendiente — 15/10/2026 (Claude)

El HD HUD queda **desactivado para los jugadores hasta estar completo**: la
opción está oculta en el juego y en el launcher, y `PortConfigLoad` lo fuerza a
0 aunque `kh_com.ini` diga `hd_hud=1`. Para desarrollarlo y probarlo, arrancar
con `KHCOM_HD_HUD_DEV=1` (entonces vale `hd_hud` del ini). Su trabajo de v0.8
rompió el texto HD de los diálogos de campo (las ventanas de mensaje marcaban
su zona como cubierta); arreglado en v1.4 en `PortPpuTextGlyphs`.

# Codex retomó — 14/10/2026, 08:47 (Santiago)

Objetivo vigente: trabajar junto a Claude hasta verificar todas las barras
al 100 %. Claude mantiene integración y releases. No publicar ni sustituir
partidas/ejecutables del usuario desde este bloque.

## Reparto al retomar

- Claude está modificando `port_ppu.c`, `src/engine.c`, `engine.h` y
  `REPARTO.md` en `khcom-claude`: conserva el cierre de headers/transiciones
  de menús. Codex no toca esos archivos mientras esté ese trabajo activo.
- Codex retoma **Gameplay verification**: contraste del replay VBM USA con
  el cartucho, sincronía tras la intro y rutas reproducibles de ambas
  campañas. Trabaja en herramientas de replay y evidencia independiente.
- El árbol `khcom` está limpio en ef3a82f3b; los cambios de la sesión anterior
  ya fueron integrados por Claude. No reintroducir versiones antiguas.
- `status.json` solo deja abiertas menús/eventos (93 %) y gameplay (85 %).
  El cuadro antiguo de REPARTO y varios informes históricos conservan
  porcentajes anteriores; leer el estado actual y sus commits de evidencia.

### Avance posterior

- Claude integró los headers y cerró menús/eventos en ae59ef83a/22733e166.
  No repetir ese trabajo. Gameplay sigue en 85 %.
- La auditoría `HDTEXT_CIERRE_AUDIT.md` identificó familias numéricas de texto
  aún sin convertir pese al 100 % de la barra. Codex toma exclusivamente
  category/value/CP de los tres archivos de barajas y `mode_status.c`;
  pruebas regionales off/on y comparación exacta del modo pixel en curso.
  Map/save, números OBJ del mapa, teclado de 64 bytes y subida de nivel
  permanecen pendientes, distintos de los números artísticos de naipes.
- `WIDESCREEN_CIERRE_AUDIT.md` no encontró un defecto nuevo de extensión;
  sí un error de contabilidad de las salas: 120 casos únicos por región,
  118 capturas de campo y dos encuentros, no 131/129. Esto no reemplaza
  las partidas completas de Sora/Riku pendientes.

## Lo que queda por demostrar en gameplay

El replay local USA de 20.000 frames terminó sin crash pero seguía en el
primer combate tutorial, nivel 1. Esto no demuestra una partida completa.
La cinta tiene 283.693 frames; antes de ejecutarla completa hay que contrastar
la misma secuencia en emulador y encontrar el primer punto de divergencia.
No inyectar progreso ni convertir batallas/eventos de debug en una campaña.

El contraste original mGBA también se desincroniza respecto de la cinta
VBA-rr: permanece en selección de archivo hasta el siguiente grupo de
inputs, mientras el port aceptó la A anterior. Por eso el tutorial diferente
en frame 20.000 no demuestra por sí solo un bug del port.

`test_tas_boot_sync.py` observa los modos originales mediante punteros de
IWRAM, sin escribir memoria. Evidencia
`build/codex-tas-boot-sync/20261014_085720_415173`: copyright2 408, título 605,
selección de partida nueva 931, todavía ahí en 1600. El mismo prefijo nativo
entra en copyright2 394, título 591, selección 820, película 948. Mover la
cinta 14 frames no resuelve el desfase de navegación: película 934. No
modificar el juego para hacer coincidir a la fuerza un TAS de otro core.

El siguiente paso es adaptar las entradas normales con checkpoints de solo
lectura. Riku tiene además un archivo SRAM original desbloqueado extraído
byte a byte de la cinta BK2 (no se editan estadísticas ni progreso Riku);
el probe inicia una campaña Riku nueva por el menú y llega al primer combate.
La continuidad y el final de ambas campañas siguen sin demostrar.

Las herramientas `test_gameplay_*` quedan como evidencia de salas, submenús,
pacing y replay; `test_rooms`, `test_submenus`, `test_ppu` y `test_options`
de Claude se conservan. Sus alcances son diferentes y no se borran por
duplicidad nominal.

Al terminar un bloque, Codex hace commit solo de sus archivos propios para
que Claude pueda integrarlo. Las capturas/ROMs/cintas/partidas permanecen en
directorios locales ignorados.
