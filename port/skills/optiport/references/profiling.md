# Profiling que permite decidir

Partir de un recorrido reproducible, no de una optimización supuesta. Identificar SHA/binario, arquitectura, backend/GPU, assets, opciones y condiciones del equipo. Comparar baseline y cambio con la misma carga y calidad; separar cold y warm y repetir lo suficiente para distinguir variación de efecto. No recomendar 64 bits por arquitectura ni comparar HD desactivado contra HD completo como si fueran equivalentes.

Si la corrida rechaza una entrada o recurso desfasado, conservar el intento como evidencia de degradación, sin mezclarlo con la medición de calidad completa. Reparar o regenerar la entrada en una copia aislada y repetir los casos afectados con su nueva huella; no relajar el validador para obtener un resultado verde. Una etiqueta de escenario necesita corroboración del estado activo, no sólo de un resumen de otro subsistema.

Instrumentar sólo las etapas necesarias para localizar el coste: lectura, decode, conversión, upload, espera de cola, cálculo, consulta, render o audio. Medir duración y trabajo pendiente para distinguir trabajo caro de espera/backpressure. Comprobar que la instrumentación no altera materialmente el recorrido; si sólo hay renderer software, limitar la conclusión a ese backend.

Usar latencia de carga, tiempos de frame p50/p95/p99 y stalls cuando existan muestras suficientes. Registrar el tamaño de muestra y la ventana: un promedio de FPS no describe regularidad ni una sesión corta demuestra ausencia de acumulación. Para memoria, distinguir RSS/private, buffers propios y VRAM; medir picos durante solapamiento de recursos además del estado estable.

Elegir un cambio ligado al cuello observado y verificar rendimiento junto con calidad, input, resultados y limpieza. Conservar baseline y resultados comparables, incluidos efectos adversos. Si no hay mejora medible, volver a la hipótesis en lugar de inventar porcentajes; resultados de un equipo no certifican otra plataforma. Para publicar, aplicar los controles pertinentes del paquete en [preparación de entrega](release-readiness.md).
