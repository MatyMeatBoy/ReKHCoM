# Pendientes después de v2.0 — 26/10/2026

**v2.0 estable publicada.** Confirmado en GitHub el 26/10/2026:
[release v2.0](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.0),
publicada a las 17:20 de Chile (`2026-10-26T20:20:29Z`, `isPrerelease=false`).
El usuario confirmó el lanzamiento y pidió limpiar la lista y preparar el tráiler.

## Retoma Codex — 27/10, robustez 2.1 y cierres (prioridad actual)

- [ ] **Prioridad: #277 Deck / #272 Android.** Investigar por separado: foto #277 = puerta de evento de Ciudad de Paso con spotlight; NO puerta del castillo. Log #272 = evento 74 Monstruo, ROM USA, Galaxy Tab S6 Lite / Android 13. Las pruebas anteriores hasta selección de mundo/tutorial no cubren la puerta fotografiada. Tres comentarios en cada issue al volver a consultar; sin log nuevo.
- [ ] **Cierre del usuario en Monstruo**, primer/segundo combate al volver a la ventana. Quiver v2.0 EU, Undub, HUD HD. Instalación principal `C:/Users/MP/Documents/Apps/QuiverLauncher/Apps/KingdomHeartsChainOfMemories-ReKHCoM`: **SOLO LECTURA, nunca ejecutar, modificar ni actualizar allí**. Copia del log/config/save en `khcom-hd-reload/build/codex-evidence/quiver-crash-20261027`. Log previo ya reemplazado al reabrir, sin dump/evento de crash reciente encontrado; RAM no confirmada como causa.
- [ ] **Repaso completo 2.1**, autorizado con decisiones técnicas delegadas: fallbacks, profiling, gestión de recursos, asíncrono/precarga, streaming y frame pacing. Algo más de peso en assets HD nuevos. Primero depurar defectos; luego alternativas recuperables, sin ralentizar lógica GBA. Alcance y orden en [PLAN_FALLBACKS_V21.md](PLAN_FALLBACKS_V21.md). No declarar ausencia global de bugs.
- [x] **Logs actual + cinco anteriores**, rotación y sesión fechada: integrado PR #282, SHA `5a523c61e`. Harness productor real, errores de rotación, shell y builds Windows/guest pasan. Guest freestanding no usa reloj libc; host conserva fecha.
- [ ] **Bucle de música HD sin progreso**: reproducción de bloqueo y corrección local RED→GREEN (decodificador vacío, dispositivo rechaza datos, reproducción normal, loop corto, loop vacío). Vuelve a música GBA sin silenciarla. Faltan integración/PR y regresión de audio completa; no atribuirlo aún a #277/#272.
- [ ] **Rendimiento HD**: nuevo arnés serial `profile_hd_resources.py` guarda frame times, cold/warm, RSS/private y hashes. Primera corrida quedó en selector de modo oculto (config de fixture=Choose); corregido a modo explícito, no tomarla por medición válida. Retomar tras atender cierres. GPU oculta y audio dummy; memoria de proceso no equivale a VRAM.
- [ ] **Protección del progreso ante fallos**: auditar guardado atómico y recuperación separada de slots manuales; no intentar continuar memoria corrupta ni afirmar que existe recuperación automática aún.

Rama propia `fix/crash-diagnostics`, checkout `khcom-crash-diagnostics`; toma remota `crash-diagnostics-20261027` / `codex-crash`. Se conserva `fix/v21-bugs` de Claude y su lista detallada. Workers autorizados por usuario, esfuerzo bajo, tareas acotadas; builds seriales/silenciosos y ninguna release nueva.

**Actualización OptiPort:** [tablero de 14 bloques](OPTIPORT_V21.md), skill [pública](https://github.com/MatyMeatBoy/OptiPort). Integrados #283–#289: layouts/bounds, sprites, paletas, alineación, readiness HUD, feed musical y vídeo Android/MF. #277 pasa entrada completa Windows/Linux64; no declarar confirmación del reportero ni #272 resuelto. Usuario autorizó integrar y enviar builds de prueba a ambos reporteros, luego continuar la batería. Host musical, vídeo Linux y preservación SRAM ante error de flush están en carriles separados.

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
