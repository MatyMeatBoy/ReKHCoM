# Robustez y rendimiento 2.1 — 27/10/2026

Mandato: revisar fallbacks, profiling, recursos, precarga/asíncrono, streaming y frame pacing; algo más de peso en assets HD nuevos. Decisiones técnicas delegadas. Primero depurar defectos, luego añadir recuperación. No escribir ni ejecutar nada en la instalación Quiver del usuario. Sin automatizaciones ni compilación paralela. Una mejora comprobada por PR.

Actualización del usuario, 27/10: autorizadas prerelease, builds para #277/#272
y ZIP personal normal de Windows limpio, sin etiqueta alpha. Final 2.1 después
de completar OptiPort y su evidencia. Reparto Codex/Claude, matriz de runtimes y
cola única en [COORDINACION_OPTIPORT_V21.md](COORDINACION_OPTIPORT_V21.md).

Prioridad actualizada: recoger/integrar los PR regulares ya listos antes del ZIP
Windows 64, entregar ese ZIP sin esperar nuevas mejoras y luego completar el
resto de OptiPort. Pendiente posterior: vista/previews en ambos modos (identificar
primero los modos en el UI); fases de profiling de entrada/p95/memoria,
caché/precarga/cancelación y calidad. Fallback visual regular a cargo de Claude,
con claim y coordinación de rutas. No implementar esta ampliación ni retrasar
el ZIP en esta fase; detalle en el documento de coordinación.

## Criterios

- Conservar el flujo y velocidad originales del cartucho. La degradación afecta presentación/calidad opcional, no la lógica.
- Recursos HD fallidos nunca autorizan a saltar historia ni mutar un guardado.
- Controlar y registrar errores de apertura, tamaño, decodificación, textura y cola de audio.
- Reintentos acotados; liberar exactamente una vez lo que se creó. Medir antes/después y distinguir memoria de proceso de VRAM.
- No afirmar que RAM/HD causó #277, #272 o el cierre del usuario sin reproducción.

## Entregas y evidencia

1. Logs: actual + cinco anteriores; stderr Linux y hosts también. RED→GREEN en productor real y shell. Build EU aislada.
2. Música: reproducir bloqueo del feed con decodificador sin progreso y cola que rechaza datos. Acotar bucles; volver a BGM GBA sin silenciarla. Normal/loop corto siguen funcionando.
3. Imágenes/HUD: validar dimensiones y presupuesto antes de decodificar; recursos activos no se expulsan. Comprobar retorno al sprite original y cachés estables.
4. Vídeo: auditar creación y destrucción, fallos de asignación, resolución y procesos ffmpeg. Validar primera imagen antes de sustituir la escena. Comprobar restauración de audio/controles.
5. Profiling: escenarios deterministas originales/HD en título, Monstruo y transición de puerta; tiempo por cuadro y pico RSS/private del proceso, versiones y opciones registradas. GPU oculta, audio dummy, serial.
6. Guardado/recuperación: auditar escrituras atómicas existentes y sus errores. No reanudar memoria corrupta ni usar un savestate incompatible como protección automática. La recuperación de progreso requiere formato/validación propios y pruebas separadas.

## Estado inicial

- Feed de música hosted entra en bucle infinito al devolver cero muestras o rechazar la cola de audio (reproducción con inyección en el productor real).
- Windows Media Foundation usa ciertos objetos sin comprobar su creación; fallo de memoria puede desreferenciar NULL.
- Sprites importados validan tamaño después de descomprimir; presupuesto de caché permite pasarse si no encuentra víctimas.
- Android devuelve éxito aun cuando no llegó a crear una textura de vídeo. Linux ffmpeg requiere revisar finalización de pipes al abandonar.
- HUD core ya valida cabecera PNG antes de decodificar y limita 4 Mpx; caché de sprites HUD 8 MiB. No duplicar protecciones existentes.
- Guardado Windows ya escribe temporal + reemplazo, pero no informa al jugador de errores. El log de la sesión del cierre de Quiver ya fue reemplazado al reabrir.

Pendientes de reproducción: #277 puerta de evento Ciudad de Paso (foto), #272 evento 74 USA/Android 13 Tab S6 Lite, usuario Monstruo primer/segundo combate con Undub/HUD HD al volver al juego. No confundir puerta del castillo con la foto de #277.
