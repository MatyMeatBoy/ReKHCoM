# Pendientes después de v2.0 — 26/10/2026

**v2.0 estable publicada.** Confirmado en GitHub el 26/10/2026:
[release v2.0](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.0),
publicada a las 17:20 de Chile (`2026-10-26T20:20:29Z`, `isPrerelease=false`).
El usuario confirmó el lanzamiento y pidió limpiar la lista y preparar el tráiler.

## Bugs a resolver antes de v2.1 estable — 27/10 (Claude)

- [ ] **#277 Deck / #272 Android: cierre tras la Llave del Inicio** (puerta del castillo → selección de mundo).
  Windows EU pasa el punto (llega al combate tutorial con A repetida). Sospecha: camino portable (host 64 bits /
  Android). Reproducir en VM Linux 64 y arreglar; luego barrer el juego entero en 64 bits.
- [ ] **#42 desync del final HD de Sora**: rama `fix/ending-cutscene-desync` (Codex), sin integrar.
- [ ] **#163 galería**: vídeo del reportero (youtube qkZSuuTsrio) sin revisar todavía.
- [x] Tests lentos con `KHCOM_FAST` + vsync=1 (el present esperaba al refresco): FAST fuerza vsync 0.
- [ ] Batería: arneses que piden builds viejos (`build/rel-eu`, `codex-gameplay-*`…) y fallos reales a separar:
  `test_hud_crop` (3840x1080 mapa), `test_ppu` 7/12, `test_door_views` (falso positivo con «VRR-path=unavailable»).
- [ ] **HUD HD activado en pleno combate**: tirones como de carga al volver de Opciones. Precargar el HUD HD
  antes de devolver el control al juego.
- [ ] **Carta de recarga HD**: el número queda debajo del relleno; debe ir encima.
- [ ] **HUD HD: «EXP ganada»** no se reubica y queda tapada por la vida nueva.
- [ ] Linux 64 VM: partida nueva → puerta del castillo → selección de mundo → combate tutorial **sin fallo**
  (sin música HD). Repetir con música HD (`.ogg`), que el reportero sí tiene.

- [ ] **Tienda Moguri (Cambiar naipes), números HD**: en la columna derecha el «2» de «02» sale deformado
  (como una Z con cola); en 3×04 y 4×01 la «x» y los dígitos se ven desalineados frente al resto. Arreglar
  directo (sin Atlas); revisar los 10 dígitos y la «x» en ambas columnas.
- [ ] **Cartas HD pequeñas (lista de naipes): encaje de la imagen** como se ajustaron las de llave. Marcadas por el
  usuario: Simba (retrato desborda/cortado), Genio (asoma por la derecha), carta verde de objeto (cubo azul
  descentrado), carta gris de enemigo (sombra/Soldado). Revisar todas las cartas, no solo esas.
- [x] Tienda Moguri en vista de tienda (fila de 5 cartas con número): números HD se ven bien (captura usuario 27/10).
- [ ] **HUD HD (solo modo HUD HD; el HUD normal no se toca):**
  - Pila de cartas: reubicarla según la vida nueva y dibujarla **encima** de ella; también se les pierde el **borde**.
  - Igual para enemigos/jefes si aplica.
  - Números de las cartas: **dorado y azul no se ven abajo**; **dorado no se ve arriba** (la azul arriba sí).
    Parece HUD mal posicionado o vista incompleta. Arreglar limpio.
  - «EXP ganada» tapada por la vida nueva.
- [ ] **Set HD de rotura de cartas** (cortanaipes, bloqueo/contranaipe y afines): el usuario recuerda haberlo
  visto listo; buscarlo en hd_sprites.tsv/ramas/Atlas y comprobar que se dibuja.
- [ ] **Cortanaipes HD** (sleight/romper carta) que se veía antes ya no aparece: revisar que se muestre **todo** el HD
  que tenemos (barrido de todas las piezas HD importadas vs. las que realmente se dibujan).
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
  Test nuevo `port/tools/test_hd_gallery_pause_text.py` (WIP, sin commit verificado): la galería no encuentra el
  evento (`found=0`) con 516 ni 513 → ajustar selección (ver `test_gallery_matrix.py`: 515/516 son alias en
  pestañas HD; usar escena con `KHCOM_GALLERY_EVENT` como lo arma esa matriz).
- [ ] **Atlas: escenas de diálogo con retrato en x4 1080p** (ej. Hades «¡Regla n.º 6!» en Coliseo: el retrato
  sobresale del cuadro y la caja no ocupa la vista amplia) nunca entraron al Atlas. Buscar las similares y enviarlas.
- [ ] **HUD HD vs jefes**: el nombre del hechizo que castea el jefe (p. ej. «15A Locura» junto a su carta) sale
  cortado/mal ubicado; ajustar posición con su pila y subir la pila del jefe para calzar con el HUD HD (arriba
  derecha) y mejorarla en HD. Solo en modo HUD HD.
- [ ] **Sonido de recarga del mazo enemigo**: casi no se oye o no suena (usuario con undub). Revisar con/sin undub.
- [x] HD de «nivel plus», match de contranaipe y bloqueo: el usuario confirma que se ven bien.

## TRASPASO (Claude, 27/10): cómo retomar

**Prioridad 1: el cierre de #277 (Deck) / #272 (Android) tras la Llave del Inicio.** Crítico; aún NO reproducido.
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
