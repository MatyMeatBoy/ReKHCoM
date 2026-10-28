# ReKHCoM 2.2 — traspaso y pendientes

## Reanudación y alcance completo — 28/10/2026

La [pasada completa de OptiPort](OPTIPORT_FULL_PASS_20261028.md) evalúa16 áreas con criterios de cierre:14 originales más contratos entre runtimes/paquetes y fiabilidad del ensayo/coordinación. 10/14 (~71%) sigue siendo el resultado histórico del alcance original; el porcentaje de esta pasada ampliada espera conciliación de evidencia, no se infiere de leer24 documentos ni de añadir bloques.

Prioridad reanudada: entregar primero el [test Linux439/441](LINUX_TEST439441.md), sin reducir la calidad de los vídeos; después completar la aplicación de OptiPort y continuar los demás pendientes. La reducción a720p queda sólo como boceto interno aplazado por el usuario, sin activación automática ni reparación de instalaciones existentes. Los reports abiertos actuales son449/441/439/412/374/330. Los PR432/440/442/443 quedaron cerrados sin merge por la orden anterior de parada: sus piezas pendientes no se vuelven resueltas por ese cierre. Main incluye444/446/447/448; los artefactos públicos2.2 conservan su fuente original. La skill completa394c34ff está revisada y sincronizada; eso no completa las16 áreas de aplicación.

Las secciones siguientes conservan los snapshots anteriores; esta actualización prevalece. La cola remota sigue siendo la autoridad de reservas.

## Estado vigente: v2.2 publicada, 28/10/2026

[Entrega y verificación publicadas](V22_PUBLISHED_VERIFICATION.md): ocho paquetes
disponibles y Web beta desplegado, fuentes/hashes por artefacto, nueve respuestas
a issues y cierres 418/421. Siguen abiertos 407/412/374/330/336/378/163 y la revisión
nueva 432. Web arrancando una ROM y los dispositivos originales conservan sus
límites de verificación. La cola remota determina los próximos claims.

Lo que sigue es el traspaso histórico del 27/10; sus frases sobre una build futura
no sustituyen el estado publicado anterior. El porcentaje 10/14 describe aquella
batería original, no todos los pendientes ni un porcentaje de estabilidad.

Estado del 27/10/2026. El usuario pidió trasladar aquí lo pendiente después
de publicar 2.1. No seguir ampliando la revisión de 2.1 ni sustituir sus
archivos publicados. Este documento organiza el trabajo; la autoridad de
reservas y responsables sigue siendo `coordination/art-queue`.

## Qué está entregado

- [2.1 estable](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.1),
  tag `8c613ae5d`: Windows x86/x64, Linux desktop/Deck x86/x64, Android beta
  ARM32/ARM64 y Anbernic armhf beta. Motores compilados en `cd144335e`;
  launcher/empaquetado Linux en `de434db7c`.
- ZIP personal en `KHCoMPC/releases/ReKHCoM-v2.1-windows-x86-x64.zip`, SHA256
  `1e1d1039b42cf3f12243afe4bd114d5d5d913b117675dd95fbbab53b78baa888`.
  La instalación Quiver principal permanece protegida: no ejecutar ni escribir allí.
- **#356 está integrado en main, pero NO está en 2.1 ni en ese ZIP.**
  Los fondos mínimos GBA/HD sin poster se incluirán en una build de 2.2.
  [Implementación y capturas](GALLERY_EMPTY_BACKDROP.md).
- #354 documentó limpieza y entrega; #355 añadió mediciones y lecciones de
  OptiPort. No confundir esos cambios posteriores con una nueva build publicada.
- Después de 2.1 se integraron #363 (nombre del hechizo del jefe), #365
  (barra completa del enemigo según su máximo), #367 (galería Sora/Riku con
  L2/R2, F2 y tooltip) y #362/#364 (presupuesto del arnés y consolidación del
  workspace). Son cambios en main; no sustituyen los paquetes 2.1 publicados.
- #368 añade recuperación y diagnóstico del modo Windows 64 bits: aviso para
  jugar en 32 bits, generar un reporte o cerrar; Automático recuerda el fallo
  para esa versión. No identifica la causa del cierre de Pedro en #360 ni
  equivale a confirmación en su equipo.

## OptiPort: porcentaje y límites

**Base conservadora: 10 de 14 bloques originales cerrados/integrados,
aproximadamente 71%; cuatro siguen parciales.** Es un porcentaje de trabajo
de aquella primera batería, no de estabilidad, bugs eliminados ni del alcance
total ampliado. No sumar porcentajes de tareas heterogéneas o declarar 100%
por haber publicado 2.1. `17f0ffa` era la referencia del método en el primer
traspaso; las mejoras posteriores del método, la organización del workspace,
la migración, los controles y el pulido se registran aparte de esos 14 bloques.

Los diez bloques integrados son logs, portabilidad/alineación, pools/paletas,
imágenes/presupuesto, readiness HUD, música, vídeo, streaming/backpressure,
lifecycle y protección del progreso. Hardware no probado sigue sin verificar.

| Bloque parcial | Evidencia existente | Cierre pendiente para 2.2 |
| --- | --- | --- |
| Profiling | Doce corridas del paquete Windows 32/64; título y combate Monstruo, HD sí/no, percentiles y RSS/private. [Datos](data/v21-initial-profile.json). | Completar sólo recorridos que cambien decisiones: puerta/carga y previews; comparación con igual calidad visual y condiciones, antes de afirmar mejoras. |
| Asíncrono y precarga | Caché/worker de previews #342 y preparación HUD #346; cancelación y resultados obsoletos cubiertos por pruebas de consumidores. | Medir entrada fría/caliente y cambios rápidos GBA/HD; comprobar presupuesto y ausencia de esperas al cerrar. |
| Frame pacing | Primeras ventanas comunes de cuadros 300..600. La variación de 32 bits no identifica por sí sola un bug. | Aislar tirones repetibles y etapa costosa antes de modificar carga o render; no alterar velocidad de la lógica GBA. |
| Entrega y resistencia | Release, manifiestos/hashes y regresiones dirigidas entregados. | Soak acotado de los recursos modificados, gameplay de los paquetes combinados finales Linux/Android y confirmación de dispositivos originales cuando esté disponible. La publicación ya está hecha. |

## Prioridades concretas

| Prioridad | Pendiente y estado real | Siguiente acción mínima |
| --- | --- | --- |
| P0 | **#360 Windows 64 bits:** Pedro informa cierre con Automático y Forzar 64 en 2.1; Forzar 32 funciona. Sin reproducción de su fallo ni confirmación del reportero. #368 integra aviso, recuperación y log de excepción. | Recibir logs/datos del equipo y contrastar el fallo concreto. Ver las reglas de Automático/Forzar 64 debajo; las pruebas de error inducido no diagnostican el equipo de Pedro. |
| P0 | **#330 Steam Deck nativo 2.1:** el reportero confirma arranque desde Konsole y fallo desde Gaming Mode tras seguir el setup; carpeta Documents en vez de Games. [Comentario posterior al cierre, 27/10](https://github.com/MatyMeatBoy/ReKHCoM/issues/330#issuecomment-6446302893). | Contrastar scripts/entrada del asset publicado y rutas con espacios; falta su log. Documents por sí solo no identifica una causa. Este reporte es distinto del Windows de #360. |
| P0 | **Android Moto Edge 60, 12 GB RAM/512 GB:** un segundo reportero dice que no inicia desde la penúltima actualización y que antes cerraba tras vencer a Jafar. [Comentario Android dentro de #360, 27/10](https://github.com/MatyMeatBoy/ReKHCoM/issues/360#issuecomment-6447762599). | Mantener separado del Windows de Pedro: faltan versión exacta, log y reproducción. No es confirmación ni fallo de la corrección Windows #368; no atribuir causa a RAM. |
| P0 | **#272 Android Monstruo:** evento74 USA pasa en pruebas locales previas, Samsung sin confirmar; el issue sigue abierto. | Usar respuesta/log nuevo del reportero; distinguir fallo nativo, lifecycle e import. No atribuir causa confirmada a RAM. |
| P0 | **Cierre del usuario al recuperar foco en Monstruo v2.0:** aún sin reproducción. Es distinto de la puerta 64 bits ya corregida. | Reproducción aislada con foco/pausa y log de sesión conservado; los logs anteriores al cierre original se habían perdido. |
| Cerrado EU x86 | **Retorno de galería sin flash:** #339 y evidencia dirigida 11/11 verificados en fuente `1c0b6c7e4`. | [Cuadros, auditorías y alcance](GALLERY_RETURN_QA_V22.md): título/partida × GBA/ReKHCoM × normal/skip, HD puro y fallback ausente/corrupto. D3D11 oculto instrumentado; no acredita otros hosts/dispositivos ni rendimiento. |
| P1 | **Dígitos premium y modificados:** #341 protege el fallback multicolor; falta comparación GPU completa. | Carta premium y efecto modificador reales, arriba/abajo, import completo/parcial. [Fixture](HUD_CARD_DIGIT_FALLBACK.md). |
| P2 | **Previews rápidas y fondo mínimo:** #342 y #356 implementados; el fondo no demuestra latencia. | Incluir #356 en la próxima build; medir caché/frío/cancelación y calidad en ambos modos. |
| P2 | **Prompts y controles experimentales:** implementados, default Original; falta QA visual y hardware. | Hotplug, bindings efectivos, todos los botones, modos Nintendo/Xbox/PS/GBA, paletas/localización y opciones no disponibles. |
| P2 | **Media EN/JP:** bibliotecas/import y selección implementados. | Contrastar las fuentes propias EN/JP y offsets, disponibilidad y wording; imports legacy siguen con idioma desconocido. No automatizar descargas Steam sin soporte verificado. |
| P2 | **Launcher Linux básico:** #353 evita depender de Wine para importar; no tiene paridad completa con Windows. | Confirmar flujo con usuario Steam Machine, dependencias y mensajes. El crash original Windows/Proton de #336 no tiene causa diagnosticada aunque exista vía nativa. |
| P2 | **Pulido regular restante:** retratos/cajas de diálogo amplias, HUD contra jefes y otros detalles sin cierre. | Mantener los tickets existentes de [pendientes](PENDIENTES_VIGENTES.md); reproducir un defecto por PR, conservar arte y revisión exacta. |
| P3 | **Linux ARM64:** builder fuente experimental; no es el binario de Deck ni Anbernic armhf. Consolas sin nuevas builds 2.1. | Construir/verificar cada plataforma que se vaya a ofrecer; publicar sólo artefactos reales y mantener beta donde corresponda. |
| P3 | **Noticias en la ficha de Steam (acceso directo no Steam):** pedido del usuario 27/10. Steam sólo muestra noticias/eventos de apps con página Steamworks; un acceso directo deja vacía esa zona y no acepta feed, archivo ni enlace. | Investigar sin prometer: (a) noticias dentro del launcher desde las releases de GitHub, (b) plugin Decky en Deck, (c) mods del cliente (Millennium) sólo como guía opcional. Verificar antes de ofrecer; no tocar la instalación de Steam del usuario. |
| P3 | **Mantenimiento de almacenamiento:** pasada de consolidación #364 cerrada; los secundarios están en `kh-worktrees/`, los paquetes en `releases/` y pruebas históricas en `pruebas/workspaces/`. | Retirar cada checkout temporal al integrar tras conservar ramas, WIP, ignorados útiles y evidencia. No contar junctions/hardlinks como copias ni descartar originales para llegar a una cifra. [Registro y límites](WORKSPACE_CLEANUP_20261027.md). |

## Cierres locales y límites de la evidencia

- **Nombre de Axel, #363:** el nombre termina antes de la pila con HUD/cartas
  HD y lienzo amplio; GBA conserva su layout. Compilación Windows64 EU y
  capturas silenciosas antes/después a 1280×720: nombre, total y tres cartas
  legibles. El repro `KHCOM_BATTLE=186:12:0` sirve de regresión; ya no es una
  tarea libre para volver a implementar.
- **Barra del enemigo, #365:** cuerpo completo según vida máxima y escala
  de 560 HP como Sora; control de enemigo débil y Axel, prueba dirigida PASS.
- **Galería, #367:** catálogos de Sora/Riku separados de GBA/HD/ReKHCoM;
  L2/R2 o F2 conservan selección/desplazamiento. Consumidor C pasa EU/US/JP;
  capturas de Sora 151/151, Riku 48/48 y tooltip Nintendo ZL/ZR revisadas.
  La revisión visual recompiló la UI sobre un core EU cacheado identificado
  por hash: no es una compilación completa ni prueba de todos los backends.
- **Arnés y workspace, #362/#364:** los discos persistentes nuevos usan 16 GiB
  por defecto; sin `--fetch` no se crea outbox de 1 GiB. Las imágenes existentes
  no se encogen automáticamente. PS5 se verificó en 16 GiB y Linux gráfico
  en 4 GiB. La foto al cierre fue 28,72 GiB en C: y 39,13 GiB en D:, conservando
  originales; no se alcanzaron 30 GiB para el proyecto completo ni se atribuye
  una mejora de velocidad a la limpieza. [Detalle](WORKSPACE_CLEANUP_20261027.md).

## #360: recuperación implementada, causa aún pendiente

En main, #368 distingue errores de carga de una salida por excepción Windows:

- **Automático:** reintenta inmediatamente en 32 bits para los códigos de
  arranque 2/3/4 sin entrar al juego. Los códigos de excepción Windows,
  incluidos los del loader clasificados en ese rango, pasan por el aviso.
  Ante una excepción de 64 bits,
  antes o después de arrancar, muestra el aviso y recuerda el fallo para esta
  versión; el siguiente inicio en Automático elige 32 bits. El botón «Jugar en
  32 bits» permite continuar desde ese aviso. No hay un umbral de segundos.
- **Forzar 64 bits:** no cambia silenciosamente a 32. Explica una instalación
  64 incompleta y, si el proceso falla, ofrece la misma elección explícita del
  aviso. «Jugar en 32 bits» sólo fuerza esa siguiente ejecución; no cambia
  la preferencia guardada Forzar 64.
- **Diagnóstico:** código de salida, información de máquina y sección de
  excepción en el log; reporte de un archivo con la sesión actual y cinco
  anteriores. Las pruebas de #368 cubren un cierre inducido, captura del aviso
  y predicados de selección. No reproducen el cierre original del reportero.

Mantener #360 abierto hasta disponer de evidencia del equipo afectado. La
petición de datos ya está publicada; no duplicar preguntas ni prometer que la
recuperación resuelve la causa. El ZIP 2.1 existente sigue siendo el publicado.

## Cómo continuar sin repetir trabajo

Tomar un claim, reproducir el pendiente específico y elegir un cambio pequeño;
verificar su consumidor y una captura cuando sea visual, abrir PR e integrar
con lock. No repetir la matriz de submenús ya capturada, recompilar todas las
plataformas por documentación ni perseguir ruido de una medición aislada.
Actualizar el porcentaje sólo al cerrar un bloque con evidencia. Mantener
separados implementado, verificado, integrado, publicado y confirmado por el
reportero. Llevar a OptiPort sólo lecciones generales comprobadas.
