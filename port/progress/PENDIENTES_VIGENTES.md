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
