# Retomar ReKHCoM y procesar la cola

Cuando el usuario diga «ponte al día y procesa la cola pendiente», empieza por
[EMPEZAR_AQUI.md](EMPEZAR_AQUI.md) y
[COLA_COLABORATIVA.md](port/progress/COLA_COLABORATIVA.md).

- Refresca `origin/main` antes de elegir trabajo. Conserva cualquier WIP ajeno;
  si el checkout está ocupado o atrasado, crea un worktree propio desde `origin/main`.
- Consulta `python port/tools/art_queue.py list`. La rama remota
  `coordination/art-queue` es la autoridad de tomas, responsables e integrador.
  La tabla de `PENDIENTES_VIGENTES.md` es una fotografía, no una reserva.
- Mantén varios carriles independientes, como pidió el usuario. Cada agente
  debe obtener un `claim` exitoso antes de editar su pieza. Un conflicto CAS
  exige refrescar y comprobar el dueño; nunca seguir como si la toma hubiera pasado.
- Registra en el momento rama, revisión humana, estado, pruebas y PR. Todos
  los cambios entran por PR; no pushear código directamente a main.
- Integra solo después de obtener el lock remoto de integrador. Si está
  ocupado, trabaja otro carril. Integra un PR a la vez, registra su SHA real
  y libera el lock al terminar; nadie lo roba por antigüedad.
- Para arte, consulta la mesa compartida y la aprobación de la versión exacta.
  Respeta la política vigente en `LOGICA_APROBADOS.md`; una hija no hereda
  aprobación. Conserva comentarios, trazos, imágenes y partidas.
- Piezas de arte: sigue [WORKER_BRIEF.md](port/progress/WORKER_BRIEF.md), sobre todo «REGLA 25/10»:
  intentar SIEMPRE (base del generador Python + mejora propia) y subir la imagen a la app antes del PR;
  una duda vuelve como propuesta con imagen, nunca como pregunta suelta. `next --owner <tu-nombre>`
  reparte sin choques; varios agentes (Claude y Codex) trabajan en paralelo y Claude integra.
- Varias escenas tocan los mismos archivos (`port_preliminar_v1_tiles.h`, `_data.inc`,
  `preliminar_v1_recipes.py`, `LOGICA_APROBADOS.md`): edita solo el bloque o la fila de tu escena,
  no regeneres el `.inc` entero, y rebasa sobre `origin/main` justo antes de abrir el PR.
- Habla en español, prueba en silencio y no publiques releases ni comentarios
  externos sin autorización. Deja traspaso antes de terminar.

Los comandos completos, recuperación explícita y pruebas están en
[ART_QUEUE.md](port/tools/ART_QUEUE.md). No iniciar otra cola local vacía.

## Revisión visual de v2.1 (instrucción del usuario, 27/10/2026)

Para todo el trabajo visual pendiente de v2.1, no se exige pasar por Atlas:
el agente coordinador revisa directamente capturas reales y corrige hasta
verificar el resultado. Esta instrucción expresa sustituye el requisito de
aprobación en Atlas para ese alcance; conserva los recursos existentes y
las partidas. No autoriza publicar releases ni editar la instalación del jugador.
