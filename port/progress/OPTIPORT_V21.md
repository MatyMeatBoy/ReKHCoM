# OptiPort — batería ReKHCoM 2.1

Skill pública y reutilizable: [MatyMeatBoy/OptiPort](https://github.com/MatyMeatBoy/OptiPort). La copia en `port/skills/optiport` identifica el método aplicado en esta revisión; los tickets y resultados del juego se mantienen aquí, no en la skill general.

## Avance comprobable

**5 de 14 bloques integrados: 36 % del trabajo definido**, no porcentaje de estabilidad ni garantía de bugs eliminados. La estimación inicial de 25 % precedía a este desglose. Un bloque termina cuando tiene revisión, verificación apropiada y entrega integrada, o una conclusión documentada que justifique no cambiarlo. La confirmación del reportero y el hardware no disponible se registran por separado.

| Bloque | Estado y evidencia |
| --- | --- |
| Logs y diagnóstico | Integrado #282: actual + cinco anteriores, error de rotación conserva evidencia; harness real, shell y builds. |
| Portabilidad/alineación | Integrado #286: ROM con alineación real y actualización de caché; regresión x32/ELF y entrada completa Windows/Linux64. |
| Pools/paletas | Integrado #285/#286: mensajes, elecciones y cartas; agotamiento inyectado y bancos válidos. |
| Imágenes y presupuesto | Integrado #284: cabeceras antes de decode, límite duro, pin/LRU y fallback de primer frame. |
| Readiness del HUD | Integrado #283/#287: bounds LAD/SEQD, grupos requeridos y texturas listas en hilo renderer; ASan/UBSan. |
| Música HD | Hosted integrado #288; seis casos incluyendo loop de 480 muestras. Host bridge: corrección adicional de loops y decoder fallido en curso. |
| Vídeo HD | Android/Windows integrado #289: primera textura/upload, skip/close y MF allocations. Linux EOF y cancelación en curso. |
| Profiling | Arnés serial oculto, hashes, cold/warm, frame times y RSS/private. Corrida GPU actual en curso; sin atribuir memoria de proceso a VRAM. |
| Asíncrono/precarga | Auditar infraestructura existente y costes medidos antes de introducir hilos; cancelación y resultados obsoletos pendientes. |
| Streaming/backpressure | Auditar buffers/colas y lectura interrumpible; Linux pipes y host musical tienen hallazgos reproducibles en corrección. |
| Frame pacing | Medir colas y p95/p99 antes de repartir trabajo; preservar velocidad GBA. Corrida software previa no representa GPU. |
| Lifecycle | Auditar foco/suspensión/device reset. Retorno de Android no debe contar pausa como timeout; invalidación de cachés GPU pendiente. |
| Protección del progreso | Auditoría encontró pérdida de dirty SRAM al cambiar de banco si flush falla. Corrección y fault injection en curso. No hay recuperación automática nueva prometida. |
| Entrega y resistencia | Pendiente: compilar SHA combinado, test ROM-free Deck/Android, hashes, regresión del paquete, recorridos prolongados y envío a #277/#272 autorizado por usuario. |

## Reportes y límites

- #277: fixture correcto de puerta de evento, dos fallos reproducidos (alineación y paleta), corregidos. Entrada hasta batalla frame 2239 y salida 0 en Windows/Linux64. Falta Deck del reportero. [Detalle](DOOR_PORTABILITY_277.md).
- #272: evento 74 USA pasa en Linux con HD apagado; no reproducción Android aún. Mensajes/paletas protegidos; no afirmar causa resuelta.
- Cierre propio en Monstruo al volver a ventana: no reproducido ni atribuido a RAM. Instalación Quiver **sólo lectura; jamás ejecutar ni modificar allí**. Los tests usan copias aisladas.
- No publicar estable 2.1. El usuario autorizó integrar y enviar builds de prueba a los reporteros; mantener claramente separadas las pruebas de la release estable y no incluir ROMs/partidas/configuraciones privadas.

Cada entrega debe actualizar PR, SHA, pruebas y pendientes. Seguir implementando y subiendo bloques mientras continúa la investigación; no esperar al final de la batería para entregar.
