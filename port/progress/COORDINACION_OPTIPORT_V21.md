# Coordinación OptiPort y v2.1 — mandato del usuario, 27/10/2026

Este documento fija el reparto autorizado desde el 27/10. Complementa
[OPTIPORT_V21.md](OPTIPORT_V21.md) y [PLAN_FALLBACKS_V21.md](PLAN_FALLBACKS_V21.md).
Es una fotografía fechada y una guía de trabajo; la autoridad de reservas y
estados sigue siendo `coordination/art-queue`, gestionada con
[art_queue.py](../tools/ART_QUEUE.md). No abrir un segundo backlog ni otra cola.

## Reparto y orden de integración

- **Codex:** terminar OptiPort al 100 % de su alcance y evidencia, preparar la
  prerelease Windows 64 y el ZIP personal limpio de Windows. Después de cerrar
  OptiPort, integrar los PR de pendientes regulares mediante el lock remoto.
- **Claude:** puede tomar **todos los pendientes regulares**, incluidos detalles
  visuales del HUD, HUD de jefes, encaje de cartas, comprobación de recetas,
  Atlas, galería y demás pendientes funcionales/visuales. Reservar cada pieza
  en la cola remota antes de editar, preservar WIP ajeno y entregar mediante PR.
- Los carriles activos de guardas del core OptiPort, arquitectura y QA conservan
  sus claims y responsables. No interferir en sus rutas ni reasignarlos por
  antigüedad. Ante solapamiento, registrar dependencia y coordinar un alcance
  disjunto antes de editar. El trabajo previo de Claude ya integrado en main
  sigue vigente; este reparto no autoriza revertirlo ni rehacerlo.

Un claim exitoso precede cualquier edición. Cada integración usa el lock remoto;
si está ocupado, se sigue con trabajo independiente. No robar tareas ni locks:
una recuperación exige comprobar sesión, worktree y PR, y dejar autorización y
evidencia mediante el protocolo de recuperación explícita.

## Registro único y prueba mínima

Para cada ID, conservar en la cola y en su evidencia enlazada: owner, rama,
scope y rutas, dependencias, reproducción, evidencia, SHA y pruebas, PR, merge,
artefacto y siguiente prueba mínima. Si un campo aún no existe, señalarlo como
pendiente; no inferir un merge o una publicación a partir de un test local.
Los snapshots de este directorio deben indicar fecha y referencia de la cola;
no sustituyen sus reservas. **Integrado no equivale a publicado; publicado no
equivale a confirmado en el dispositivo del reportero.**

La primera pasada original fue **9/14 bloques, aproximadamente 64 %**, según el
aviso del usuario. Es un hito histórico de alcance, no un porcentaje de
estabilidad. La migración 32→64 es alcance añadido y se sigue por separado,
con sus propias dependencias, artefactos y verificaciones. El cierre requiere
evidencia de todos los bloques vigentes; no promete ausencia total de bugs.

## Entregas autorizadas y matriz de arquitectura

El usuario autoriza ahora publicar una **prerelease**, enviar builds de prueba
a los reporteros de **#277 y #272**, y entregar su **ZIP personal normal de
Windows**, limpio y sin etiqueta alpha. Esta autorización sustituye la
prohibición temporal anterior de publicar otra release. No constituye evidencia
de que ya se haya publicado o enviado nada. La versión final **2.1** requiere
completar OptiPort y sus verificaciones.

| Destino | Entrega y verificación requerida |
| --- | --- |
| Windows | Selección de runtime Auto / Forzar 32 / Forzar 64; prerelease 64 y ZIP personal normal. Comprobar selección y fallback real. |
| Linux desktop / Steam Deck | Un paquete con runtimes 64/32; comprobar selección y arranque en ambos destinos. |
| Android | APK universal con 32+64; comprobar ABI seleccionada, instalación y ruta relevante. |
| Consolas | Mantener condición beta y documentar evidencia y límites por plataforma. |

Comprobar interoperabilidad de SRAM manual y configuración entre runtimes;
los quicksaves necesitan espacios de nombres separados y comprobación explícita
de compatibilidad. No reutilizar un quicksave incompatible ni dar por probado
un dispositivo por una ejecución de VM o Windows. Para #277/#272 distinguir
reproducción local, build enviada y confirmación del reportero.

## Protección del entorno y cierre

La instalación principal
`C:/Users/MP/Documents/Apps/QuiverLauncher/Apps/KingdomHeartsChainOfMemories-ReKHCoM`
está protegida: no ejecutar, probar, actualizar ni modificar allí. Usar copias
y salidas aisladas; conservar partidas, configuración, arte y WIP existentes.
Los cambios entran mediante PR, sin pushes directos de código a main.

Prueba mínima siguiente: verificar la evidencia pendiente de OptiPort y la
matriz de runtimes contra los SHA exactos que se empaquetarán; registrar hashes
de ZIP/APK/paquetes, resultados y límites antes de afirmar entrega o cierre.
Los pendientes regulares pueden avanzar en paralelo bajo claim; su integración
por Codex queda después del cierre de OptiPort.

## Orden del espacio de trabajo después de 2.1

Pendiente de cierre: inventariar worktrees, agrupar los útiles bajo
`KHCoMPC/khcom-worktrees` según las raíces de cada repositorio, y retirar basura
y checkouts cerrados innecesarios. Desde ahora preferir esa carpeta para nuevos
worktrees. No mover los activos ni borrar nada durante esta fase documental.
El repositorio fuente independiente de OptiPort no es un worktree de ReKHCoM.

Al cerrar cada tarea registrar simplemente **WorktreeNeeded: sí/no** y el motivo
(revisión, reproducción, PR o artefacto pendiente). Si está integrado y validado,
preservar primero WIP, partidas, ROMs, evidencia y commits; copiar/archivar la
evidencia necesaria y retirar después con Git. No forzar borrados de archivos
del usuario. Antes de futuros movimientos o borrados recursivos en Windows,
resolver las rutas absolutas y comprobar que permanecen dentro del workspace
previsto. Mover un checkout invalida cachés de builds con rutas absolutas:
regenerarlas. No limpiar SDK, dependencias compartidas, instalación principal
ni partidas. Este registro usa la misma cola; no crea una autoridad adicional.
