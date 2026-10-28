# Profiling que permite decidir

Partir de un recorrido reproducible, no de una optimización supuesta. Identificar SHA/binario, arquitectura, backend/GPU, assets, opciones y condiciones del equipo. Comparar baseline y cambio con la misma carga y calidad; separar cold y warm y repetir lo suficiente para distinguir variación de efecto. No recomendar 64 bits por arquitectura ni comparar HD desactivado contra HD completo como si fueran equivalentes.

Si la corrida rechaza una entrada o recurso desfasado, conservar el intento como evidencia de degradación, sin mezclarlo con la medición de calidad completa. Reparar o regenerar la entrada en una copia aislada y repetir los casos afectados con su nueva huella; no relajar el validador para obtener un resultado verde. Una etiqueta de escenario necesita corroboración del estado activo, no sólo de un resumen de otro subsistema.

Instrumentar sólo las etapas necesarias para localizar el coste: lectura, decode, conversión, upload, espera de cola, cálculo, consulta, render o audio. Medir duración y trabajo pendiente para distinguir trabajo caro de espera/backpressure. Comprobar que la instrumentación no altera materialmente el recorrido; si sólo hay renderer software, limitar la conclusión a ese backend.

Usar latencia de carga, tiempos de frame p50/p95/p99 y stalls cuando existan muestras suficientes. Registrar el tamaño de muestra y la ventana: un promedio de FPS no describe regularidad ni una sesión corta demuestra ausencia de acumulación. Para memoria, distinguir RSS/private, buffers propios y VRAM; medir picos durante solapamiento de recursos además del estado estable.

Elegir un cambio ligado al cuello observado y verificar rendimiento junto con calidad, input, resultados y limpieza. Conservar baseline y resultados comparables, incluidos efectos adversos. Si no hay mejora medible, volver a la hipótesis en lugar de inventar porcentajes; resultados de un equipo no certifican otra plataforma. Para publicar, aplicar los controles pertinentes del paquete en [preparación de entrega](release-readiness.md).

## Vista profunda de tirones

En software por cuadros, si las métricas agregadas no localizan un tirón,
instrumentar las etapas pertinentes del cuadro: lógica, composición,
presentación y operaciones puntuales como lectura, decode, upload, shaders o
glifos. Definir sus límites: una medida llamada lógica no debe incluir el
autosave anterior ni la escritura del propio logger. Distinguir trabajo CPU,
espera y ejecución GPU; una llamada de presentación puede bloquear.

Conservar el mayor coste observado y su recurso/operación, además de los
acumulados necesarios: el último recurso cargado no es necesariamente el más
caro ni la causa. Un pico aislado sigue siendo evidencia que investigar;
repetir la carga y contrastar actividad concurrente antes de atribuirlo a un
mecanismo. No exigir repetición para conservar un fallo excepcional relevante.

Acotar líneas, resumen de peores cuadros e historial. Con la sonda apagada,
evitar lecturas de reloj, contadores, nombres y E/S de diagnóstico; comprobar
esa ruta además de medir el overhead activo. Si hay captura/readback o logging
que contamina la muestra, separar esa corrida de la comparación de rendimiento.
Medir no demuestra mejora: verificar después el cambio con carga y calidad
equivalentes, sin alterar lógica, orden o presentación para obtener cifras.

## Cadencia de media frente a cuadros de pantalla

Si el tirón pertenece a vídeo, separar cadencia/PTS y duración del contenido, avance del reloj elegido, cuadros decodificados/en cola y la identidad/PTS del cuadro realmente presentado. Los FPS de la ventana no cuentan cuadros nuevos del vídeo: mantener un cuadro de una fuente de menor cadencia puede ser correcto, y una imagen repetida puede estar en el contenido original. Medir intervalos y cuadros perdidos/repetidos respecto a sus timestamps, además del coste de lectura, decode, conversión, upload y espera; un gameplay fluido no identifica el cuello del vídeo ni demuestra un fallo de codec. En lecturas fragmentadas, contar también intentos sin datos y espera por intento: un sleep fijo puede acumular el coste de un frame; contrastar espera por disponibilidad con timeout/cancelación acotados, conservando entrada, formato y calidad.

Contrastar el mismo recurso y ruta de importación con clocks de audio/vídeo disponibles, pausa, salto y fallback pertinentes. Separar falta de datos de presentación tardía, cadencia esperada y reloj detenido; comprobar continuidad al cambiar de reloj cuando termine el audio. Comparar calidad y sincronización junto con latencia antes de cambiar codec, resolución o buffering. Separar selección de contenido, resolución, compresión/bitrate y cadencia del perfil de importación; contrastar salidas reales y manifiesto/receta/generación, incluidas importaciones heredadas, sin inferir una transformación por el nombre del preset. No acelerar el contenido ni la lógica autoritativa para igualar los FPS del display; registrar qué etapa está medida y cuál sigue pendiente en el dispositivo.
