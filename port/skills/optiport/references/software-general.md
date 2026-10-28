# Protocolo para software general

Aplicar a la aplicación real y a su flujo solicitado. Presentar el trabajo exclusivamente con lenguaje profesional de software: operación, resultado, latencia, recursos, compatibilidad y evidencia. Mantener este criterio en documentación, ejemplos y comunicaciones futuras.

## Preparar una comparación válida

1. Identificar el flujo: abrir un documento, navegar una vista, ejecutar un cálculo, consultar datos o exportar. Fijar entradas y resultado esperado antes de cambiar código.
2. Registrar baseline: SHA o versión, configuración, dependencias, sistema, CPU/GPU, conjunto de datos y condición de caché. Proteger originales e instalación activa; usar copias aisladas para pruebas destructivas.
3. Medir las etapas que pueden explicar el coste. Distinguir tiempo total, espera, trabajo CPU, transferencia y presentación. Repetir baseline y cambio en condiciones equivalentes, alternando el orden cuando el entorno pueda sesgar el resultado.
4. Elegir un cambio acotado ligado al cuello observado. Comparar resultados y comportamiento además de tiempo y memoria. Separar caché fría/caliente, primer uso y uso sostenido.
5. Guardar comandos, resultados brutos y resumen con tamaño de muestra, dispersión, errores y límites. No inventar percentiles con pocas muestras ni atribuir toda una mejora a un cambio si cambiaron también datos, calidad o entorno.

| Registro mínimo | Contenido |
| --- | --- |
| Identidad | Flujo, versiones antes/después, equipo, configuración y entradas identificables sin publicar datos privados |
| Correctitud | Resultado esperado, comparación exacta o tolerancia ya autorizada, unidades y exportación pertinente |
| Medición | Repeticiones, ventana, caché fría/caliente, latencia mediana y colas/percentiles si hay muestras suficientes, memoria estable y pico |
| Recursos | Métricas diferenciadas de proceso, GPU y buffers; método y limitaciones de captura |
| Resultado | Valores antes/después con unidades, variación y efectos adversos; cambios implementados y comprobaciones pendientes por separado |

No establecer una mejora porcentual sin valores comparables y denominador explícito. Una corrección funcional puede estar verificada aunque el rendimiento siga sin medirse; comunicar ambos estados por separado. Este protocolo no certifica por sí mismo ninguna aplicación.

## Mecanismos y pruebas que deben conservarse

- **Memoria y caché:** limitar bytes y entradas, contar recursos simultáneos y proteger los activos. La clave incluye identidad, revisión, parámetros, unidades y contexto pertinente; invalidar y liberar sin duplicar ownership. Verificar presión, fallo parcial, expulsión y recarga.
- **Carga asíncrona:** colas acotadas, cancelación y generaciones por documento o vista. Publicar sólo resultados de la revisión activa; respetar afinidad del hilo de UI/GPU. Probar cambios rápidos, cierre durante carga y consumidor lento.
- **Ciclo de vida:** reiniciar contextos y dispositivos, pausar/reanudar y cerrar sin esperar indefinidamente. Recursos de generaciones anteriores no pueden reaparecer tras su invalidación.
- **Transacciones:** comprobar escritura, validación y reemplazo antes de cambiar el estado en memoria. Ante error conservar documento anterior, estado pendiente y capacidad de recuperación; verificar también cancelación y falta de espacio.
- **Presentación y exportación:** reducir trabajo de vista sólo dentro del contrato permitido. Modelos, coordenadas, unidades, IDs, undo y resultado final conservan sus invariantes. Una vista visualmente similar no valida un archivo exportado.

En un reemplazo que requiere varias asignaciones, adquirir y validar el candidato completo antes de publicar sus alias y liberar el recurso anterior, cuando el presupuesto permita su coexistencia. Si falla la segunda asignación, descartar sólo el candidato parcial y conservar el anterior usable; probar cada rechazo con el consumidor real y un control válido. Si el presupuesto impide coexistir, definir antes de retirarlo una ruta de degradación, reconstrucción acotada o salida controlada; no publicar `ready` ni dereferenciar un reemplazo fallido. Para objetos con preparación diferida, aplicar la [admisión del recurso y sus alias](verified-lessons.md#objetos-creados-con-preparación-diferida).

## Ejemplos de aplicación, pendientes de medición propia

| Aplicación | Hipótesis que comprobar | Evidencia necesaria |
| --- | --- | --- |
| Visor web 3D | Evitar trabajo repetido o limitar recursos inactivos puede reducir latencia/memoria | Misma vista, trayectoria y datos; captura de tiempos y memoria; selección y coordenadas conservadas |
| CAD o editor | Invalidación incremental puede evitar recálculos ajenos a una edición | Mismo documento/edición; dependencias afectadas; equivalencia del modelo y exportación |
| Procesamiento de datos | Lotes acotados y backpressure pueden limitar picos | Mismas entradas/orden requerido; resultados equivalentes; throughput, pico y cancelación |
| Aplicación de escritorio | IO asíncrona puede reducir bloqueos de interfaz | Mismo archivo y operación; respuesta de UI, duración total, persistencia y cierre seguro |

Estos ejemplos orientan pruebas; no afirman ganancias observadas. Usar [capas de verificación](verification-layers.md) y [preparación de entrega](release-readiness.md) según el cambio y el alcance prometido.
