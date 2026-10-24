# Reglas del usuario (cómo trabajar en ReKHCoM)

Sintetizadas de sus pedidos hasta el 24/10/2026. Hablarle en español, breve y directo.

## Cola y avance
- Una sola cola: `PENDIENTES_VIGENTES.md`. Anotar cada avance en el momento (qué, commit, cómo se probó, qué falta) y
  dejar un bloque TRASPASO arriba antes de que se corte la sesión.
- Subir a main cada pieza verificada enseguida (no juntar varias).
- Hacer exactamente lo pedido; lo chico y verificado se decide y se cierra solo; preguntar solo lo grande (arte,
  lógica del juego, publicar).
- "Aprobado con mejoras" = cerrado ahora; las mejoras son a futuro, no pendientes.
- Cuando aparece un bug, reproducirlo, arreglarlo y buscar sus parientes en todo el juego (DEBUG_WORKFLOW.md). Un
  timeout o cuelgue en una batería es un bug: replicarlo hasta resolverlo. Nada de crashes en una estable.

## Juego
- Velocidad original del cartucho (una actualización por VBlank, 59,7275 Hz); nunca modos de velocidad.
- No inventar objetos ni vidas del juego desde el port: arreglar dentro del flujo del juego (`#ifdef PLATFORM_PC`).
- Toda mejora (QoL) opcional: en Opciones del juego, el launcher y `kh_com.ini`. Estética de los menús del propio juego.
- Lo aprobado va encendido para todos; una variable de prueba solo lo apaga (`=0`), nunca lo enciende.
- Arte de vista ancha: se entrega como copia jugable + .bat con antes/después y espera aprobación
  (`PRUEBAS_VISUALES.md`, `WORKFLOW_ARTE_AMPLIO.md`); lo aprobado se anota en `LOGICA_APROBADOS.md`. Pruebas de arte a
  x3 (zoom=6) en 1920x1080 y 3840x1080; el rectángulo GBA debe quedar idéntico.

## Pruebas
- Nunca abrir ventanas visibles ni sonar (el usuario juega a la vez): `SDL_VIDEO_DRIVER=dummy/offscreen`,
  `SDL_AUDIO_DRIVER=dummy`. Cerrar emuladores, VMs y procesos de prueba apenas terminan.
- Baterías largas en segundo plano y reanudables (`run_batteries.py --detach --resume`).
- La validación en hardware real no es un pendiente: llega como reporte. Si un reporte no dice plataforma, equipo,
  sistema y versión, pedírselo al reportero en el issue.
- No borrar las copias de prueba del usuario (`jugar/ReKHCoM-vX`): juega y guarda ahí.

## Builds y publicación
- No hacer builds de todas las plataformas ni de lanzamiento hasta que el usuario dé el OK; las de prueba
  (prerelease) son solo Windows salvo que se pruebe otra plataforma.
- Publicar (releases, comentarios en issues) solo con su OK. Notas y comentarios breves; nunca decir en público que
  algo no se probó en hardware; agradecer por ser parte del proyecto de fans.
- **No borrar ni reemplazar nunca un release ni un archivo publicado** (Quiver Launcher nos aceptó el 24/10 y borrar
  parecía sospechoso). Guardar los originales de cada estable fuera del repo.
- Estable: PC (Windows, Linux, Steam Deck) estable; el resto `-beta`. La v2.0 sale **sin Switch** (vuelve con el motor
  nuevo comprobado, `SWITCH_MOTOR.md`).
- Antes y después de publicar: `release_check.py vX` y `--post`; `test_update.py`; publicar la web.

## Repo
- Que cualquiera con su ROM pueda retomar desde GitHub (`EMPEZAR_AQUI.md`): instrucciones como texto, nada de
  imágenes de VM ni binarios grandes en git; mantener el repo liviano.
- Codex/ChatGPT también trabaja en main: si el otro lleva más de 10 minutos sin tocar nada, se pueden tomar sus
  puntos anotándolo en la cola; no tocar su árbol de trabajo.
