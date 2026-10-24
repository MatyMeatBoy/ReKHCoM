## CIERRE DE WORKERS Y COLA ÚNICA (vigente)

Abrir [PENDIENTES_VIGENTES.md](PENDIENTES_VIGENTES.md): 44 puntos y responsables.
Cierres comprobados en [CIERRE_WORKERS.md](CIERRE_WORKERS.md).
Primero recoger los workers actuales; no abrir otra tanda.
Los apartados siguientes son historial y no otra cola vigente.

## ENCARGO VIGENTE: CLAUDE + CODEX, CERRAR Y PUBLICAR (18/10/2026)

**Claude:** guardados/persistencia + savestate borde superior + savestate 3
checkerboard + integración y publicación autorizada. **Codex:** 057/061/074
en vista amplia. Asignación y checklist: [REPARTIJA_20261018.md](REPARTIJA_20261018.md).
Esta asignación sustituye repartos previos de estas tareas.

## Repartija actualizada para Claude — 18/10/2026

Ver [REPARTIJA_20261018.md](REPARTIJA_20261018.md): guardados, persistencia,
savestates y reparto de workers. Estado vigente: **1/44 escena aprobada (León),
3 mejorables**; revisión con lienzo mucho más amplio antes de continuar.
Este encabezado sustituye los contadores antiguos de aprobación.

## Integración Nightly 2 — 18/10/2026

Guard Armor aceptado y perfiles de escenas 057/061/074 conectados al juego,
incluyendo playa mejorada `b651e4b65`. EU/US/JP compilan; escenas 23/23 y
Guard Armor 13/13 PASS. Detalles: `NIGHTLY2_INTEGRATION.md`. Preparada como
`v1.9.3-nightly.20261018.2`, sin promover a stable. Siguiente jefe: Darkside.

## Guard Armor aceptado e integrado — 18/10/2026

**1/8 escenarios de jefe terminados.** Continuación de patio por tiles/píxeles
originales, solo fuera de GBA. EU/US/JP compilan; 13/13 comparaciones, Original
y rectángulo GBA idénticos; Hook/Monstro sin cambios. Receta reutilizable en
`BATTLE_SCENERY_TECHNIQUE.md`. Usuario pide integrar además los commits de
escenas del otro chat en una nueva nightly y después continuar con Darkside.

## Actualización: auditoría de escenarios de combate — 18/10/2026

Inventario y contador en [BATTLE_SCENERY_AUDIT.md](BATTLE_SCENERY_AUDIT.md),
con estado estructurado en `BATTLE_SCENERY_AUDIT.json`.
**28/28 escenarios distintos inspeccionados inicialmente; 0/8 mejoras terminadas.**
56/56 ejecuciones cortas EU/Sora en 16:9 y 32:9 (168 capturas), más 8/8
largas de Hook/Jafar/Oogie/Marluxia final (24 capturas). No se modificó el juego.
El usuario considera que Hook y Monstro ya se ven bien: conservarlos.
Alicia/Trickmaster: solo extender hacia abajo el ajedrez, sin rehacer la sala.
Otros siete pendientes: Guard Armor, Darkside, Oogie, Jafar, Maléfica dragón,
Úrsula y Marluxia final. No rellenar por defecto los vacíos de todos los mapas.
Las capturas no certifican todas las fases, campañas ni regiones.

La nightly `nightly-2026.10.18` ya está publicada como pre-release; la estable
sigue en v1.9.2. #12 tiene respuesta pública con la limitación de gameplay
pendiente. RetroAchievements sigue pendiente de implementación; esta auditoría
no sustituye ese encargo.

## Codex — 18/10/2026

Encargo completo en **port/progress/CODEX_TAREAS.md**: issues #14 (Undub desde
la ROM JP, prioridad), #13 (reasignar controles, gatillos), #6, respuestas de
#5/#4/#3, menú de opciones con el selector del debug oficial, truco "modo
debug", revisar unstock, #12, tira del fondo en arenas GBA y RetroAchievements.
Codex puede integrar a `main` y responder issues con pruebas verificadas;
Claude prepara releases y Pages. No cerrar issues sin autorización del usuario.
Claude sigue con los fondos de escenas en lotes de 10 con el usuario.

## Reparto vigente — 18/10/2026 (sustituye a los anteriores)

### Actualización Codex — 18/10/2026

La rama local `codex/issues` contiene el primer bloque de issues y mejoras del
encargo en [CODEX_TAREAS.md](CODEX_TAREAS.md); el detalle de estado y pruebas
está al final de ese archivo. Build Windows EU/US/JP PASS; launcher 11/11,
Re:CoM 80 combinaciones, zoom 56 y aspecto 56 PASS. Undub está implementado;
`port/tools/test_undub.py` verifica dumps de dos voces en EU/US (OFF conserva
el audio regional; ON produce el mismo audio entre regiones). #12 queda tras
Opciones > Experimental y apagado por defecto; el test del menú/config pasa,
pero falta probar guardar ambas ranuras en gameplay. El nightly no será stable.
La tira de las arenas y la integración de
RetroAchievements siguen pendientes. Codex integró este bloque
en `main` (commit `ae40c79de`), sin
crear paquete ni release. Próxima versión anotada para preparación: **v1.9.3**;
la última estable pública sigue siendo v1.9.2.

El usuario autorizó a Codex a responder issues con cambios probados. Ya se
publicaron respuestas en #4 (56 casos de zoom, incluido 2880×1800), #5 (16:10
1280×800 → 256×160 a ×5) y #6 (multiplicadores Beginner/Proud). Siguen abiertos;
no se cerró ninguno ni se creó una release.

El usuario confirmó como referencia el repositorio abierto
[RaceWave46](https://github.com/DomazinUS/RaceWave46) (GPL-3.0). La
investigación y límites de los sets están en
[RETROACHIEVEMENTS.md](RETROACHIEVEMENTS.md); no se debe presentar el diseño
de popup como una integración funcional hasta que estén conectados los sets.

Estable pública: **v1.9.2** (Windows + Android alpha + Web alpha, un archivo
por plataforma). Todo lo demás en borrador. Issues #7, #8 y #9 cerradas.

### Esta sesión (con el usuario, revisión visual)
1. Escenas 16:9: extensión del suelo con tiles originales (PortSetBgIsoExtend,
   lista kIsoScenes en port_ppu.c). 44 de 194 eventos cambian con KHCOM_ISOALL;
   muchos mapas tienen un vacío oscuro a propósito. Revisar en lotes de 10 con
   antes/después y cuadros de cámara; solo se añaden a la lista las aprobadas.
   Aprobada: 004 (Sora conoce a Leon). Informe: build/scene-review/run1/report.json.
2. Arenas tamaño GBA (Guard Armor, Dragon Maleficent): asoma una tira del fondo
   en el borde derecho.
3. Issues rápidas: #5 (16:10 ya cubierto por la vista Automática: responder),
   #4 (más niveles de zoom: comprobar los pasos actuales y responder).

### Sesión Linux (local_2950e992, parada por límite de uso)
4. Linux x86 (PC y Steam Deck, i686 con librerías incluidas) y Anbernic
   (armhf, probablemente salida directa a /dev/fb0). Ver el plan en su sesión.

### Sesión nueva: menú de opciones, trucos y mejoras PS2
5. Selector del menú de opciones con el efecto del menú de depuración oficial
   (al que se entra con CodeBreaker EU 320348DC 0006, US/JP 3203493C 0006).
6. Trucos: primera opción "Entrar al modo debug".
7. Revisar la implementación de devolver cartas acumuladas (unstock, de
   ggdrn/KH_COM_VITA) y mover estas mejoras a un título propio dentro de las
   mejoras PS2, como el de Experimental.
8. Resto de trucos y editor de cartas (región + banco + ranura, checksum).

### Sesión nueva: RetroAchievements (investigación)
9. Ver si el port puede usar rcheevos con los logros de la versión GBA:
   como variante/hack de la ROM o al menos funcionando en local. No se puede
   enlazar como la GBA original; documentar qué se puede y qué no.

### Sesión nueva: issues de jugadores
10. #12 elegir ranura de guardado en el juego, #6 texto de la dificultad
    Orgulloso, #3 falta el texto grande del HUD, #2 funciones de la historia del
    remake (valorar alcance antes de prometer nada).

### Usuario
11. Aprobar la limpieza: worktrees khcom-fix y khcom-codex-perf (ya en main),
    rama remota codex/ppu-performance, carpetas build/ de revisiones viejas
    (~30 GB) y el checkout khcom (su trabajo sin commitear está en
    backups/khcom-hdhud-wip-20261014). Nunca las carpetas ReKHCoM-vX.
12. Pruebas en dispositivos reales: Android ARM, web en móvil, Steam Deck.
13. Partidas completas de Sora y Reverse/Rebirth (validación de jugabilidad).

### Opcional / más adelante
14. Revisión visual de menús a 21:9 y 32:9 (muy opcional).
15. Tráiler nuevo con material de la versión terminada.

Cerrados hoy: pantalla de título (difuminado, se da por terminada), zoom x3
(no había error), commits PPU de Codex (ya estaban en main), bugs @bug de la
decomp revisados (2 corregidos para PC), lógicas atadas a la pantalla GBA
(solo los enemigos del campo, corregido en v1.9.1).

## Titulo — correccion de enfoque del usuario (17/10/2026)

- Usuario rechazo el cristal/guante imaginado de ec2493eb7. Eliminados del
  arbol actual el parche y la receta anteriores; NO integrar esa propuesta.
- Nueva referencia entregada solo como guia del difuminado, NO para
  convertirla a pixel art ni pegarla en el juego. Se hizo un ensayo privado
  de alineacion, descartado; NO es el asset actual ni entra en el motor.
- Propuesta vigente: difuminar el ORIGINAL hacia blanco en los ultimos seis
  pixeles derechos/inferiores, usando su propia paleta y cuadricula nativa.
  Receta fade_title_edges.py, asset solo mascara gris en assets/title-study.
- Verificado: rectangulo234x154 identico, posiciones intactas,654 pixeles
  modificados en los bordes, colores de paleta original, bordes finales blancos.
- Preview privado build/title-widescreen-review/fade-study/
  title-original-faded-preview.png. MAQUETA, NO integrado, sin release nueva.
- Revisar resultado visual antes de integrar; luego paleta viva/fades y BG
  exclusivamente, sin modificar texto/menu/sprites. v1.9 publica intacta.

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

## Cheats / Riku: implementacion PC comprobada (17/10/2026)

- Trucos > Desbloquear a Riku, OFF por defecto; persiste cheat_unlock_riku.
  Abrir F1 en titulo ANTES de START; si ya se abrio el selector, reiniciar.
- Permiso separado del progreso: no escribe GAME_FLAG_SORA_CLEAR ni SRAM.
  Menus FULL con cheat y Sora no completado omiten Link Battle tanto en dibujo
  como en navegacion. Partidas Riku validas pueden continuarse sin cheat activo.
- PC EU/US/JP: 6/6 arranques reales con ROM privada. OFF mantiene menu original,
  ON permite seleccionar Riku. Tras seleccion flags=264 (Riku+dark lock),
  sin bit32 de Sora completado. Informe build/riku-access-report.json.
- Menu real EU: abrir Trucos, alternar No->Si, volver y pulsar START muestra
  selector Sora/Riku; INI guardada. Capturas revisadas visualmente.
- Zoom: 41 casos PASS, default=8 (x4), persistencia del cheat ON/OFF PASS.
  Controles: 12 acciones + rechazo de V/right-stick antiguos PASS.
- NO publicado; solo rama release/v1.9. Android/Web necesitan recompilacion
  y validacion del cambio antes de distribuirse. No usar APK/ZIP viejos como prueba.
- Usuario aclara x3 en BAT v1.9 Windows. INI de ESA instalacion indica zoom=8,
  fullscreen=1, escritorio nativo. No se reprodujo cambio a x3; sigue investigacion.

## Publicacion: solo estables publicas (17/10/2026)

- Nueva instruccion expresa: v1.8.3 es la UNICA release publica estable.
- v1.9 y web-alpha2 pasadas a borrador privado, conservando assets y tags.
  Verificado con gh release list: ambas isDraft=true; v1.8.3 latest=true.
- Ninguna nueva candidata debe publicarse como prerelease visible. Solo tags
  publicos para desarrollo; archivos de trabajo locales o borradores privados.
- v1.9.1 NO autorizada a salir hasta terminar/revisar los pendientes acordados.

## Pedidos vigentes: controles, zoom y Riku (17/10/2026)

- Retirar totalmente devolver cartas acumuladas: eliminados comportamiento,
  solicitud de combate, controles y opciones. Configuracion/metadatos antiguos
  no pueden reactivarlo. Considerar SOLO en Cheats en el futuro; no implementado.
- x4 sigue como default (ID interno 8) en motor y launcher. La instalacion local
  v1.9 tiene zoom=8. No convertir selecciones explicitas del usuario a otro zoom.
  Pendiente revisar limite de altura: una ventana de 540px limita x4 a x3
  para conservar el lienzo minimo GBA; no confundir este limite con un default x3.
- Desbloqueo anticipado de Riku sigue pendiente: aun NO existe Cheats en paquete
  publicado. Implementar sin marcar Sora completado ni desbloquear Link Battle.
- Usuario indica completar pendientes ANTES de generar siguientes versiones.
  Assets v1.9 publicados aun son anteriores a esta retirada; reconstruir y validar.

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

El overlay táctil nativo está conectado; falta validar campo, combate y lifecycle. No anunciar Android disponible.
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

# Reparto vigente — v1.8.1 (16/10/2026)

- [x] Estado: usar fondos para márgenes de entrada/salida, sin extender avisos NUEVO.
- [x] Derivar el comportamiento del modo actual, sin una bandera que se pierda al cargar estados.
- [x] Quitar pasos intermedios; conservar x4 predeterminado e indicaciones x6/x3.
- [x] Migración de selecciones intermedias antiguas y 41 casos de zoom.
- [~] Comparación visual entrada/salida, avisos activados/desactivados, 16:9/32:9.
- [ ] Publicar v1.8.1 y preparar acceso local.

---

# Reparto vigente — v1.8 (16/10/2026)

Codex integra y publica por pedido del usuario. Claude sin créditos.

- [x] Ocultar «Solo escalas enteras» y «Escalado» en juego y launcher.
- [x] Mantener todos los pasos; anotar x6 como íntegra x1 con aviso y x3 como íntegra x2.
- [x] x4 actual predeterminado, también en ultrawide; migrar sin perder zoom explícito.
- [x] EXP: números originales siempre; quitar HD solo de esos avisos. Posición original conservada.
- [x] Reproducir ranuras 1/2/3 de v1.7.1 sin alterar los originales.
- [x] Relleno de mapa exterior en ranuras 1/3: imagen original idéntica, 16:9 y 32:9.
- [x] Mensajes sin BG: márgenes según fondo renderizado y sus fades.
- [x] Matriz de 32 configuraciones y 3 avisos EXP; HD activado/desactivado idéntico, EU/US/JP y estados nuevos pasan.
- [x] v1.8 pública, assets/digests y latest verificados; carpeta y acceso local preparados.

---

# Reparto vigente — v1.7.1 (16/10/2026)

Codex asume integración y publicación por pedido del usuario; Claude sin créditos.
Referencia actual: CODEX_COORDINACION.md. Los bloques anteriores son históricos.

- [x] Integrar rendimiento Codex sobre interfaz y escalado v1.7 de Claude.
- [x] Capturar ranura 1 de v1.6.7 en un directorio aislado.
- [x] Corregir interpolación PCM fuera de muestra: 24 regresiones pasan.
- [x] Optimizar síntesis PSG: 800.000 integrales idénticas a la referencia.
- [x] Preparar buffer de audio antes de reproducir y al cargar un estado.
- [x] Reducir espera activa; descartar omisión de copia tras detectar regresión ultrawide.
- [x] Recuperar numeración de píxeles v1.4, predeterminado x4 a 1080p y filtro real de enteras/intermedias.
- [x] Verificación regional, estados y capturas contra paquete público v1.7; seis casos Auto y 116 pruebas de zoom.
- [x] Publicada v1.7.1 unificada; ZIP/SHA-256 verificados y acceso local preparado.
- [x] Retiradas 14 releases del lote solicitado; todos los tags remotos intactos.

---

# Entrega v1.5 — reparto vigente (15/10/2026)

El usuario pide dejar continuidad para Claude y pushear cada bloque. Esta sección
reemplaza los repartos históricos de abajo. Rama local `claude/windows-5`, remoto
`origin/codex/v1.5-fixes`. No trabajar simultáneamente sobre render/menús.

**Leer primero [CONTINUIDAD_V15_HUD.md](CONTINUIDAD_V15_HUD.md).**
Codex entrega el bloque actual; Claude puede asumir los pendientes siguientes.
No se ha publicado v1.5 ni retirado v1.4. Guardados del usuario intactos.

- [x] Zoom: normal x4 a 1920×1080; pasos efectivos distintos, test_zoom.py en seis resoluciones. Push deaabb7.
- [x] Arena: continuar textura inferior antes de componer sprites; Ciudad de Paso, Agrabah y País de las Maravillas revisados. Push b0359b4.
- [~] Minimapa: anclaje de sprites a esquina implementado; queda matriz visual de todos los zooms/HUD. Incluido en deaabb7.
- [~] Síntesis: BG1 declarado UI para evitar panel perdido y bloques cian; captura de puerta real a 1920×1080 revisada, panel recuperado.
- [~] Selector: reaplicar desplazamiento de 8 líneas tras tres redibujados; guardia de división por cero en barra de una página. Falta revisión específica tras tutorial de puerta de evento.
- [~] Puerta: eliminado recorte rectangular del sombreado; entrada/salida ordinaria capturada. Validar todos los encuadres y suavidad del haz.
- [~] Submenús: escala entera independiente x5 a 1080p / x6 a 1440p. **Cabeceras/pies desacoplados pendientes**; se retiró el intento defectuoso que cortaba la imagen en franjas.
- [ ] Claude: corregir animación de salida de fondos/márgenes; no declarada terminada.
- [ ] Claude: matriz visual 16:9, 21:9 y 32:9, especialmente 5120×1440; todas las escalas válidas; EU/US/JP.
- [ ] Claude: resolver/atribuir tres fallos del test PPU de oclusión HUD, ver guía.
- [ ] Claude: changelog/README, ZIP sin ROM, acceso local v1.5, publicar y retirar v1.4 tras validar sustituta.
- [ ] Claude: tráiler nuevo con material de la versión terminada. No reutilizar capturas antiguas como evidencia.

Las compilaciones regionales y capturas se detallan en la guía. Los estados `[~]`
son parciales y no equivalen a aceptación visual del usuario.

---

# Reparto de pendientes â€” reordenado el 14 de octubre de 2026

Responsabilidades acordadas con el usuario para trabajar sin solaparse,
ordenadas por las barras de `status.json` que faltan para el 100 %.
`[x]` hecho y verificado, `[~]` hecho sin verificar del todo, `[ ]` pendiente.

## CoordinaciÃ³n (14/10, 04:05)

- 14/10: Codex sin tokens. Mientras no vuelva a trabajar, Claude asume tambiÃ©n
  sus pendientes (widescreen field/battles/menus & events, nÃºmeros HD y su
  revisiÃ³n visual). Cuando Codex retome, se reparte de nuevo aquÃ­.

- Claude es el integrador principal: integra los commits de ambos en
  feat/native-movies, compila EU/US/JP antes de subir y prepara las releases.
  Solo se sube lo que compila en las tres regiones.
- NÃºmeros HD: Codex los cubre todos con el estilo 2 (tiles de BG en barajas y
  sprites de EXP); Claude no duplica. Pendiente: revisiÃ³n visual de colores.
- Herramientas de verificaciÃ³n: Codex tiene test_gameplay_*, test_ppu_conformance
  y test_wide_world_* sin subir; Claude ya subiÃ³ test_rooms, test_submenus,
  test_ppu, test_options y test_songs. Al integrar se unifican (sin borrar
  ninguna) y se anota aquÃ­ cuÃ¡l queda como referencia de cada barra.
- Ãšltimo: partida completa / TAS. DespuÃ©s del 100 %, la funciÃ³n experimental
  con las QoL de la versiÃ³n de PS2 (Re:Chain of Memories) en la de GBA.

## Reparto actual (14/10, tarde) â€” todas las barras al 100 %

CoordinaciÃ³n vigente por peticiÃ³n del usuario: Codex trabaja en el HUD HD y
deja commits locales con evidencia; solo Claude integra, pushea y actualiza
las releases. Prioridad del HUD: preliminar completo antes del ajuste fino.

| QuiÃ©n | Trabajo |
|---|---|
| Claude, junto con el usuario | Las 17 ventanas con mÃ¡rgenes laterales, una por una con su revisiÃ³n (lista abajo). Base: las versiones de la maÃ±ana del 14/10; solo mejoras puntuales. |
| Codex | Seguir mejorando el modo Re:CoM y sus QoL (port_ps2qol.c, la ventana de nueva partida en port_ui.c, kh_com_recom.ini). Regla: con el modo apagado o en la versiÃ³n Original nada cambia respecto al cartucho. |

### Las 17 ventanas (Claude + usuario, en orden)

Avance Codex (14/10): controles QoL y texto rÃ¡pido guardados por separado
por partida/banco en `kh_com_recom.ini`, compatible con registros anteriores.
La dificultad conserva daÃ±o cero y valores negativos. VerificaciÃ³n:
`python port/tools/test_recom.py` (80 combinaciones, cinco bancos, reinicio,
registros antiguos y aislamiento de Original). Sin cambios en los mÃ¡rgenes.

Nuevo experimental a cargo de Codex: HUD HD redibujado con IA y alfa real.
Formato preparado: atlas PNG RGBA + JSON, validaciÃ³n y vistas claro/oscuro
con `port/tools/pack_hd_hud.py`. Piloto integrado del panel HP enemigo con
acabado suavizado, PNG cacheado y opciÃ³n `hd_hud` en ambos menÃºs. Usuario
autoriza placeholders. Ver `HUD_HD_EXPERIMENTAL.md`; pruebas aisladas con
`test_hd_hud.py` superadas en EU/US/JP: recurso ausente y vista Original
idÃ©nticos a la referencia; barra/letras verdes conservan pÃ­xeles y posiciones.
El marco toma lÃ­mites nativos por frame, siguiendo la precisiÃ³n solicitada
por el usuario. No activar piezas con fondo residual o halos.

Base actual sincronizada: prerelease v0.8 y commits de mÃ¡rgenes/README del
14/10. Codex continÃºa encima: extracciÃ³n local de referencias HUD (25 piezas
en combate bÃ¡sico), dÃ­gitos de naipes con contornos CFF de KHMenu y oclusiÃ³n
por sprite. Barras propias en `status.json` â†’ `experiments`, sin modificar
el 100 % del port base. El panel enemigo preliminar ya incluye letras
HP/VT/LP/PV transparentes con relieve y relleno suavizado que conserva el
recorte nativo. PV tiene la esquina superior derecha de la P cerrada en
diagonal. Pruebas aisladas superadas en EU (es/de/it y Riku), US y JP;
pendientes: retratos, panel del jugador y restantes familias del inventario.

1. Diario â€” revisada (14/10)
2. Ver barajas / AÃ±adir-Quitar â€” revisada (14/10)
3. Estado â€” revisada (14/10)
4. Naipes mapa â€” revisada (14/10)
5. Tienda moguri â€” revisada (14/10)
6. Naipes mundo (quitar el tramado) â€” revisada (14/10)
7. Mapamundi â€” revisada (14/10)
8. Elige un mundo â€” revisada (14/10)
9. VÃ³rtice â€” revisada (14/10)
10. Bonus Ã©lite (selecciÃ³n de cartas) â€” revisada (14/10)
11. MenÃº START â€” revisada (14/10)
12. SÃ­ntesis de estancias (puerta y carta) â€” revisada (14/10)
13. Guardar â€” revisada (14/10)
14. Subida de nivel â€” revisada (14/10)
15. SelecciÃ³n de idioma â€” revisada (14/10)
16. Copyright y tÃ­tulo â€” revisada (14/10)
17. PelÃ­culas y entrada al combate â€” revisada (14/10)

## Barras pendientes y dueÃ±o (actualizado 14/10, 08:40 â€” total 98,9 %)

| Barra | % | Peso | DueÃ±o | QuÃ© falta para el 100 % |
|---|---|---|---|---|
| Gameplay verification | 100 | 10 | Claude | Cerrada por decisiÃ³n del usuario: las partidas completas quedan al feedback de los jugadores |

Cerradas hoy por Claude (con evidencia en port/progress/evidence/):
- Widescreen: battles 100 % â€” arenas US/JP 19/19, salida de combate con el
  fixture `battlewin` (combate de depuraciÃ³n y de campo), entrada con el
  corazÃ³n en su proporciÃ³n (battles_wide.txt).
- Widescreen: field maps 100 % â€” todas las salas de historia en EU/US/JP
  (118/120 salas Ãºnicas cada una; las otras dos son un encuentro antes de la captura,
  revisadas aparte), vestÃ­bulos y Bosque de los 100 Acres (fieldmaps_wide.txt).

Ya al 100 %: build, assets, memoria, BIOS, DMA, interrupciones, entrada, m4a,
PSG, PCM, cÃ³dec de bloques, pelÃ­culas, SRAM, reset, widescreen renderer,
widescreen field maps, widescreen battles, widescreen menus & events, estados
guardados, launcher, saltar
intro, versiones USA y japonesa, menÃº de opciones, PPU, texto HD.

## Acabado panorÃ¡mico (era de Codex; lo lleva Claude mientras Codex no vuelva)

- [x] Cristal del castillo: doce ciclos del menÃº conservan una sola tarea.
- [x] Cian en selecciÃ³n de carta de puerta; mÃ¡rgenes negros.
- [x] Entrada a combate: fondo difuminado, animaciÃ³n central en su proporciÃ³n.
- [x] Arenas como cinta de tiles: bandas de sus propios tiles elegidas por
  arena (btl_map.c), sin duplicar jefes ni motivos Ãºnicos; EU/US/JP revisadas.
- [x] Subida de nivel: entrada natural desde un combate de campo revisada
  fotograma a fotograma (sin pop, barras de borde a borde). El fondo que
  reaparecÃ­a a la izquierda al salir era defecto del port: el BG1 del panel
  daba la vuelta al mapa de 256 px; corregido con PortBgClip.
- [x] MenÃº de sÃ­ntesis de salas: panel 6 lÃ­neas por encima de la barra inferior.
- [x] Entradas/salidas de los seis submenÃºs (Ver barajas, Mapamundi, Naipes
  mapa, Naipes mundo, Estado, Diario) revisadas cada 3 fotogramas en EU; la
  salida de Ver barajas coincide con la GBA (el panel de naipes activos
  desaparece de golpe tambiÃ©n en el original). Mapamundi con footer completo.
- [x] Naipes mundo y Mapamundi en US/JP (fixture de sala + modo directo).
- [x] Cabeceras: en US/JP el margen mostraba letras ocultas del sprite (y, al
  pasar el tÃ­tulo a BG, columnas copiadas del cuadro). Ahora la banda usa el
  par de columnas mÃ¡s opaco y sin letras, guardado mientras la cabecera sigue en
  pantalla. Naipes mundo, Mapamundi, Estado, Diario y Barajas revisados en EU/US/JP.
- [x] Tabs del ojo en Estado y Barajas (EU/US/JP) y escenas con estirado
  (Bonus Ã©lite, Tienda moguri, Elige un mundo, VÃ³rtice) revisadas: barra al 100 %.
- [x] Sin estirado de pÃ­xeles: Diario, Elige un mundo, VÃ³rtice, Tienda moguri y
  Bonus Ã©lite usan mÃ¡rgenes hechos por el PPU con sus propias capas: el
  fondo (la capa BG mÃ¡s baja) sigue en espejo con sus tiles y paletas, como
  el flip horizontal del hardware; las capas de interfaz solo continÃºan como
  bandas donde llegan al borde. Sin tramado ni columnas estiradas.
- [x] Widescreen field: recorrido por mundo completo (arriba).

## Claude: verificaciÃ³n, regiones, texto HD, opciones y cierre del PPU

- [x] Gameplay verification: baterÃ­a de salas (test_rooms.py) 52/52 en EU: cada
  mundo en una planta temprana y una tardÃ­a y los vestÃ­bulos de las 13 plantas.
  Hoja de capturas en build/roomtest_eu/rooms.png para la revisiÃ³n de Codex.
- [x] Gameplay verification: estrÃ©s de submenÃºs (test_submenus.py) sobre una
  partida real: las seis entradas del menÃº de pausa, cuatro idas y vueltas
  cada una, 6/6 sin crash. Pendiente aparte: avisos de tutorial delante del
  texto de las cartas (tutorial de naipes mapa).
- [x] Gameplay verification: stutter de la subida de nivel medido con
  KHCOM_FRAMELOG: 16,7 ms estables, sin tirones ni fotogramas de lÃ³gica perdidos
  (renderizador por software; los picos vistos antes eran carga de otra prueba).
- [x] Gameplay verification: salas de Riku (Reverse/Rebirth) 88/88 en EU y de Sora
  en EU/US/JP 118/120 salas Ãºnicas (las otras dos, un encuentro antes de la captura).
- [ ] Gameplay verification: partida completa de Sora y de Reverse/Rebirth en
  las tres regiones (rutas reproducibles y problemas anotados).
- [x] USA/JP: combates JP 209/209 como Sora y como Riku; eventos 192/194 (los dos
  restantes se cortaron al parar, no fallaron). Barra al 100 %.
- [x] USA/JP: capturas regionales de arenas, salas, Naipes mundo y Mapamundi.
- [x] HD text: inventario (port/progress/HDTEXT_INVENTARIO.md).
- [x] HD text: nÃºmeros de barajas y EXP en HD con su color y tamaÃ±o (barra al 100 %).
- [x] Graphics options menu: test_options.py, 35 casos y el menÃº en EU (cinco
  idiomas), US y JP. Barra al 100 %.
- [x] PPU: regresiÃ³n de escenas (test_ppu.py, 12 escenas con referencia
  revisada). Barra al 100 %. Si un cambio de composiciÃ³n altera una escena,
  revisarla y regrabar con --update.
- [ ] PreservaciÃ³n: reproducir los casos de BUGS_GBA.md en EU/US/JP y separar
  tÃ©cnicas originales, anomalÃ­as del cÃ³digo y defectos del port.
- [x] Launcher e instalaciÃ³n: baterÃ­a 11/11.
- [x] Audio: comparaciÃ³n con emulador (mÃºsica, vÃ­deos), baterÃ­a de las 637
  canciones (port/progress/songs_report.txt), detalles del PSG, audio suave.
- [x] Estados guardados deterministas (3/3).
- [x] Releases v0.5, v0.6 y v0.7 (pre-releases pÃºblicas).
- [~] QoL: devolver cartas acumuladas y bancos de partidas hechos; aviso de
  puntos moguri, revancha de jefes y repetir tutoriales pendientes (no suman
  barras; despuÃ©s de las barras).
- [~] HD-2D experimental (versiÃ³n alternativa): profundidad de campo, bloom y
  gradaciÃ³n publicados; el pop-up de diorama estÃ¡ en un stash (bded017)
  y vuelve despuÃ©s de las barras.

## Reglas compartidas

- No modificar a la vez los mismos archivos; Codex trabaja principalmente en
  map/map_tasks/map_cell, card_level_up, perfiles de menÃº, PPU y presentaciÃ³n.
  Claude trabaja en harness y pruebas, opciones, launcher, audio y texto HD.
- Cada uno hace commit solo de sus propios cambios (por bloques si un archivo es
  compartido) y no toca el trabajo sin commitear del otro.
- No sobreescribir ejecutables o estados que el usuario estÃ© usando. La build
  compatible con sus estados estÃ¡ en build/priority-reference; las ranuras
  3/4/7/8/9 recientes en build/pcwork/states.
- Probar en directorios aislados y con pocos procesos mientras el usuario juega.
  No modificar partidas/configuraciÃ³n del usuario.
- No subir ROM ni recursos extraÃ­dos al repositorio.
- Subir porcentajes solo con evidencia y distinguir implementado de verificado.
- Usar emuladores Ãºnicamente para contrastar bugs o comportamientos dudosos del
  original. Validar acabados visuales y transiciones directamente en el port.
