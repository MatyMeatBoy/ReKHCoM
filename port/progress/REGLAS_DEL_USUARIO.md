# Reglas del usuario (cómo trabajar en ReKHCoM)

Sintetizadas de sus pedidos hasta el 24/10/2026. Hablarle en español, breve y directo.

## Cola y avance
- **Actualización 24/10: [COLA_COLABORATIVA.md](COLA_COLABORATIVA.md) manda sobre los repartos anteriores.**
  Mantener varios carriles con piezas distintas; tomar y registrar la pieza antes de editar. Cada entrega va en
  rama propia y pull request; un solo integrador con reserva remota fusiona los PR verificados. Si no hay
  integrador, tomar esa reserva antes de integrar. No pushear directamente a `main`.
- Una sola cola: `PENDIENTES_VIGENTES.md`. Anotar cada avance en el momento (qué, commit, cómo se probó, qué falta) y
  dejar un bloque TRASPASO arriba antes de que se corte la sesión.
- Subir la rama y abrir un PR por pieza verificada enseguida (no juntar varias); registrar PR y evidencia en la cola.
- Hacer exactamente lo pedido; lo chico y verificado se decide y se cierra solo; preguntar solo lo grande (arte,
  lógica del juego, publicar).
- "Aprobado con mejoras" = cerrado ahora; las mejoras son a futuro, no pendientes.
- Cuando aparece un bug, reproducirlo, arreglarlo y buscar sus parientes en todo el juego (DEBUG_WORKFLOW.md). Un
  timeout o cuelgue en una batería es un bug: replicarlo hasta resolverlo. Nada de crashes en una estable.

## Juego

- Excepción directa 25/10: el usuario aprobó en la app las nuevas versiones de
  102, 115, 116, 117 y 118 y pidió «integra los aprobados antes de ese zip».
  Integrar esas versiones exactas aunque estén aprobadas sin honores; conservar
  su valoración y registrar sus comentarios como mejoras de la siguiente hija.
  Esta autorización no aprueba automáticamente otras piezas ni futuras hijas.
- Velocidad original del cartucho (una actualización por VBlank, 59,7275 Hz); nunca modos de velocidad.
- No inventar objetos ni vidas del juego desde el port: arreglar dentro del flujo del juego (`#ifdef PLATFORM_PC`).
- Toda mejora (QoL) opcional: en Opciones del juego, el launcher y `kh_com.ini`. Estética de los menús del propio juego.
- Arte nuevo: activar para todos solo la versión `build_version` **con honores** de la app; aprobado sin honores
  conserva su valoración y espera ★. Una variable de prueba solo apaga la integración (`=0`). Las integraciones
  históricas siguen registradas como tales; una hija nueva no hereda la aprobación de su madre.
- Arte de vista ancha: se entrega como copia jugable + .bat con antes/después y espera aprobación
  (`PRUEBAS_VISUALES.md`, `WORKFLOW_ARTE_AMPLIO.md`); lo aprobado se anota en `LOGICA_APROBADOS.md`. Pruebas de arte a
  x3 (zoom=6) en 1920x1080 y 3840x1080; el rectángulo GBA debe quedar idéntico.

## Pruebas
- Nunca abrir ventanas visibles ni sonar (el usuario juega a la vez): `SDL_VIDEO_DRIVER=dummy/offscreen`,
  `SDL_AUDIO_DRIVER=dummy`. Cerrar emuladores, VMs y procesos de prueba apenas terminan.
- Baterías largas en segundo plano y reanudables (`run_batteries.py --detach --resume`).
- La validación en hardware real no es un pendiente: llega como reporte. Si un reporte no dice plataforma, equipo,
  sistema y versión, pedírselo al reportero en el issue.
- No más `.bat` ni carpetas `jugar/` por versión (25/10): solo el zip limpio en KHCoMPC. Antes de borrar una
  carpeta con partidas, copiar sus partidas a `respaldos/saves/`.

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
- Claude y Codex trabajan en ramas/worktrees separados. La reserva remota de la cola decide quién tomó cada
  pieza; 10 minutos sin commits no autorizan a pisar una reserva viva. Recuperar una reserva vencida con el
  protocolo de COLA_COLABORATIVA y anotar el relevo; nunca tocar el árbol sin commitear del otro.
- Logros (25/10): lo que cambia el juego (partidas, bancos de partidas, trucos, editar saves) bloquea los logros
  oficiales o pasa a la lista de Re:CoM; gráficos, audio, escenas y controles no importan. Los trucos
  (salvo mejoras del modo Re:CoM) llevan al «Modo rebelde» (logros locales y aparte).
