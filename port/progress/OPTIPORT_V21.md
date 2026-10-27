# OptiPort — batería ReKHCoM 2.1

Skill pública y reutilizable: [MatyMeatBoy/OptiPort](https://github.com/MatyMeatBoy/OptiPort). La copia en `port/skills/optiport` identifica el método aplicado en esta revisión; los tickets y resultados del juego se mantienen aquí, no en la skill general.

## Avance comprobable

**10 de 14 bloques originales integrados: aproximadamente 71 % del trabajo de
la primera batería**, no porcentaje de estabilidad ni garantía de bugs eliminados.
Snapshot de coordinación del 27/10/2026; reservas y estados vivos en
`coordination/art-queue`. El hito previo 9/14 (~64 %) se conserva como referencia
histórica. La migración 32→64, la matriz de plataformas y los nuevos controles,
galería y previews son alcance añadido fuera de este denominador. No convertir
ese 71 % en avance de todo el proyecto.

| Bloque original | Estado y evidencia |
| --- | --- |
| Logs y diagnóstico | Integrado #282/#298: actual + cinco anteriores y fecha UTC; conservar evidencia de fallos de rotación. |
| Portabilidad/alineación | Integrado #286. Seguimiento adicional #318: primera puerta con SRAM real expuso lectura BIOS GBA/NULL en Windows64; corrección validada por coordinador y PR todavía pendiente de integración en este snapshot. |
| Pools/paletas | Integrado #285/#286: agotamiento inyectado, mensajes, elecciones y cartas. |
| Imágenes y presupuesto | Integrado #284/#296: cabeceras antes de decode, límites/pin/LRU y presupuesto de targets HUD. |
| Readiness del HUD | Integrado #283/#287/#297: bounds, grupos requeridos e invalidación explícita del HUD. |
| Música HD | Integrado #288/#291/#295: progreso acotado, host bridge y límites del loader. |
| Vídeo HD | Integrado #289/#293: primera textura/upload, fallos MF y lectura/cancelación Linux. |
| Profiling | Parcial: arnés serial, cold/warm, hashes, tiempos y RSS/private. Completar escenarios representativos; memoria de proceso no equivale a VRAM. |
| Asíncrono/precarga | Parcial: medir entrada/caché y auditar cancelación/resultados obsoletos antes de introducir trabajo adicional. Previews de ambos modos son ampliación separada. |
| Streaming/backpressure | Integrado #293/#291: pipes Linux y alimentación musical host con progreso/cancelación acotados. |
| Frame pacing | Parcial: quedan mediciones p95/p99 y colas en escenarios apropiados, conservando velocidad GBA. |
| Lifecycle | Integrado #309/#311: generaciones bridge e invalidación de recursos nativos en device loss; hardware real y pausas visuales se documentan aparte. |
| Protección del progreso | Integrado #290/#314: preservación de SRAM ante flush fallido y estados atómicos. No promete recuperación automática de memoria corrupta. |
| Entrega y resistencia | Parcial: paquete privado y repro guardado pasan 32+64; faltan cierre de evidencia, soak dirigido y entrega pública/confirmación de reporteros. |

No se exige completar una campaña entera: usar fixtures deterministas por patrón
y soak dirigido que cubran los riesgos de cada cambio. Mantener el caso real
que descubrió el fallo junto a las pruebas sintéticas.

## Snapshot de builds y entrega privada — 27/10

- El coordinador informa build Windows `017676`: EU/US/JP en 32 y 64 bits
  compiladas correctamente; compilación no equivale a regresión jugable.
- El preview nativo 64 falló con la SRAM real en la primera puerta (AV WER
  `0x100f7bd6`, `MapWallFaceIsUnreserved`, fila superior NULL/lectura BIOS GBA).
  La corrección alcanzó `field=520`, salida 0; #318 sigue pendiente de integrar.
- Paquete privado con fixture: primera puerta guardada PASS en 32+64. El paquete
  inicial pasó además ocho salas y puerta de evento en ambos runtimes. El ZIP
  final pasó la regresión del guardado en ambos. No extender la evidencia de
  ocho salas a un artefacto distinto sin repetirla o justificar equivalencia.
- ZIP limpio personal copiado por coordinador a la raíz del proyecto solicitada
  por el usuario. SHA-256:
  `be1ed897f4e98c7be88ab6113692578b25b81b91d9f02823a64d7b49dd17fc81`.
  No hay prerelease pública publicada en este snapshot.

## Reportes y límites

- #277: fixture correcto de puerta de evento, dos fallos reproducidos (alineación y paleta), corregidos. Entrada hasta batalla frame 2239 y salida 0 en Windows/Linux64. Falta Deck del reportero. [Detalle](DOOR_PORTABILITY_277.md).
- #272: evento 74 USA pasa en Linux con HD apagado; no reproducción Android aún. Mensajes/paletas protegidos; no afirmar causa resuelta.
- Cierre propio en Monstruo al volver a ventana: no reproducido ni atribuido a RAM. Instalación Quiver **sólo lectura; jamás ejecutar ni modificar allí**. Los tests usan copias aisladas.
- No publicar estable 2.1. El usuario autorizó integrar y enviar builds de prueba a los reporteros; mantener claramente separadas las pruebas de la release estable y no incluir ROMs/partidas/configuraciones privadas.

El corpus local de SRAM contiene **un caso único**; no describirlo como una
colección amplia de partidas. GameFAQs es un catálogo candidato de saves
públicos: no descargados ni validados, acceso/obtención bloqueado por ahora.
No contar esos candidatos como cobertura ni distribuir ROMs/datos privados.

Cada entrega debe actualizar PR, SHA, pruebas y pendientes. Seguir implementando y subiendo bloques mientras continúa la investigación; no esperar al final de la batería para entregar.
