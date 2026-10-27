# Coordinación OptiPort y v2.1 — mandato del usuario, 27/10/2026

Este documento fija el reparto autorizado desde el 27/10. Complementa
[OPTIPORT_V21.md](OPTIPORT_V21.md) y [PLAN_FALLBACKS_V21.md](PLAN_FALLBACKS_V21.md).
Es una fotografía fechada y una guía de trabajo; la autoridad de reservas y
estados sigue siendo `coordination/art-queue`, gestionada con
[art_queue.py](../tools/ART_QUEUE.md). No abrir un segundo backlog ni otra cola.

## Reparto y orden de integración

- **Codex:** terminar OptiPort al 100 % de su alcance y evidencia, preparar la
  prerelease Windows 64 y el ZIP personal limpio de Windows. La última
  instrucción del usuario introduce una excepción al orden inicial: recoger e
  integrar los PR regulares ya listos antes del ZIP, con revisión y lock remoto;
  entregar Windows sin esperar nuevas mejoras y después completar el resto de
  OptiPort. Mandato posterior: continuar hasta 2.1 con todos los pendientes y
  PR verificados, también regulares; no esperar al cierre de toda la batería
  para integrar correcciones listas. La revisión visual de 2.1 pasa por el
  coordinador directamente, según la excepción expresa de AGENTS.md.
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
Los pendientes regulares pueden avanzar en paralelo bajo claim y entrar mediante
PR revisado y lock remoto. El ZIP prioritario no exige esperar funciones nuevas;
una regresión reproducida del alcance anunciado sí bloquea darlo por listo.

## Pendiente: vista y previews en ambos modos

Solicitud posterior del 27/10: mejorar vista/previews en **ambos modos**, con
fallback visual cuidado, carga casi instantánea y poca pérdida de calidad, sin
entradas a escenas con esperas largas. Es trabajo pendiente, sin implementación
en esta entrega documental y sin retrasar el ZIP Windows 64 prioritario.

Antes de implementar, identificar en el UI actual cuáles son los «dos modos»
citados y registrar sus nombres, rutas y comportamiento. No adivinar que son
HD/GBA u otra pareja, ni bloquear la entrega actual esperando una aclaración.

- **Carril OptiPort de rendimiento:** reservar en la cola única una tarea con
  fases: identificar modos y baseline de entrada; medir latencia p95 y memoria;
  estudiar caché, precarga y cancelación; comparar resultado con calidad visual
  y evidencia antes/después en ambos modos. Registrar latencia y memoria reales
  para fijar el presupuesto; «casi instantáneo» expresa el objetivo, no una
  garantía ya medida. No avanzar lógica de escenas para ocultar esperas.
- **Carril regular UI de Claude:** reservar el fallback visual y la presentación
  de previews; coordinar las rutas/dependencias con el carril de rendimiento,
  sin tocar sus guardas activas. Conservar contenido y aportar revisión visual.

El repositorio público de referencia es
[OptiPort](https://github.com/MatyMeatBoy/OptiPort). Es un repositorio fuente
independiente; no convertirlo en otro backlog de reservas de ReKHCoM.

## Pendientes adicionales del usuario — 27/10

- **Retorno de galería sin destello intermedio:** al terminar o saltar una escena,
  regresar directamente al modo galería/escenas, sin un cuadro fugaz del título
  o gameplay ni su fade. Una sola tarea regular UI para Claude, coordinada con
  pacing OptiPort si comparte rutas. Reproducir desde título y partida, final
  normal y skip, modos GBA y ReKHCoM; capturar los cuadros de transición.
  Restaurar UI de origen, estado del renderer, inputs y audio sin avanzar
  historia. No ocultarlo con un fade largo ni alterar timings a medida.
- **Layouts completos de prompts de mando, experimental 2.1:** pendiente regular
  UI, sin implementación actual. Selector Auto / Original-GBA / Nintendo /
  Xbox / PlayStation; tutorial y prompts deben coincidir con el binding lógico
  efectivo. Permitir elegir convención Nintendo A / PlayStation círculo;
  reutilizar assets de glyphs existentes para aceptar A cuando correspondan.
  Incluir todos los botones GBA requeridos por Opciones, overrides del usuario,
  hotplug y localización. Registrar matriz de bindings/layouts y evidencia antes
  de integrar; no asumir equivalencia por la posición física del botón.

Estas ampliaciones quedan anotadas y no bloquean el ZIP. **Sí bloquea la entrega
final del ZIP el repro guardado real de la primera puerta en Windows 64:** el
coordinador confirmó RED, WER `0x100f7bd6`, `MapWallFaceIsUnreserved`, fila
superior NULL por lectura BIOS GBA. El coordinador corrige la fuente compartida;
el ZIP debe esperar GREEN del mismo guardado/repro en la build exacta. No
presentar los tests sintéticos previos como sustituto de esa verificación.

Actualización del coordinador, 27/10: #318 integrado y repro guardado GREEN.
El paquete e71 pasa seis regresiones Windows (guardado, ocho salas y puerta de
evento en 32 y 64), y ambos recorridos de puerta en Linux32/64 VM. Android se
valida por separado. Estado vigente y hashes en [OPTIPORT_V21.md](OPTIPORT_V21.md);
no duplicar aquí un resultado de una build anterior como si fuera actual.
No hay prerelease pública nueva todavía. El corpus local es una SRAM única;
GameFAQs solo figura como catálogo candidato no descargado/validado. La batería
usa patrones, fixtures deterministas y soak dirigido; no requiere campaña completa.

Requisito persistente de entrega personal de este proyecto: colocar los ZIP
limpios en `C:/Users/MP/Documents/00 Claude/KHCoMPC`, visibles en esa raíz;
`build/releases` puede conservar salidas de trabajo pero no sustituye la entrega.
El coordinador ya copió allí el ZIP corregido; consultar su SHA y fuente vigente
en el tablero de artefactos, no asumir que coincide con el primer ZIP privado.
Esta ruta personal pertenece a las notas ReKHCoM, no a la skill pública OptiPort.

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
