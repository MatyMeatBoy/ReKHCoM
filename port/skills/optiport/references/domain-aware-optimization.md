# Optimización según el dominio

Aplicar al optimizar software de ingeniería, diseño, datos, ciencia o cualquier otro ámbito solicitado. Inferir uso, formatos, algoritmos y restricciones desde el proyecto; preguntar sólo si una decisión relevante queda ambigua. Optimizar el software conservando sus resultados verificables, sin sustituir la validación profesional de un cálculo real.

## Definir qué debe conservarse

| Ámbito | Invariantes y controles pertinentes |
| --- | --- |
| CAD/CAE/BIM e ingeniería | Geometría/topología, resultados numéricos, unidades, coordenadas, metadatos y precisión de formatos. Usar tolerancias documentadas o acordadas, casos de referencia y golden outputs validados. No inventar tolerancias ni aceptar más error sólo por ganar velocidad. |
| Diseño y herramientas creativas | Libertad de edición, color/perfiles, fuentes, layout, capas/edición no destructiva, undo/redo, resolución y fidelidad de exportación. La optimización no exige que el algoritmo sea «creativo» ni debe limitar decisiones del usuario. |
| Datos, ciencia y otros | Identificar el resultado autorizado: esquema, orden relevante, exactitud/estabilidad numérica, semillas, trazabilidad o reproducibilidad según el uso. Elegir equivalencia exacta o una métrica de error con criterio existente/acordado; no importar invariantes de otro dominio automáticamente. |

Separar **preview/presentación** de **resultado autoritativo**. LOD, resolución adaptativa o aproximaciones pueden mejorar interacción sin alterar el modelo/original. El export debe usar el resultado exacto o una equivalencia validada conforme al objetivo del usuario; si el export es una malla aproximada, su error también requiere criterio explícito. No exportar una caché de preview por accidente ni esconder pérdida de calidad.

## Localizar y reducir trabajo

Aplicar [profiling](profiling.md) al flujo real: interacción, recompute, lectura, render y export pueden tener cuellos distintos. Investigar invalidación incremental y task graphs para recalcular sólo dependencias afectadas, conservando orden y sincronización. Claves de caché deben cubrir inputs, parámetros, unidades, versión y contexto que cambien el resultado; cancelar trabajo obsoleto sin aplicar resultados de otra revisión del documento.

Evaluar bibliotecas y rutas CPU/GPU por precisión, determinismo necesario, límites de recursos y fallback correcto, además de tiempo. Comprobar error y resultados adversos con referencias, no sólo apariencia. Conservar originales y undo al probar fallos, cancelación o memoria limitada. Las [fuentes opcionales](domain-sources.md) sirven para investigar mecanismos, sin dependencia de red para este método.

## Casos de decisión

- **CAD lento al orbitar:** medir tessellation/render frente a recompute; bajar LOD sólo de preview si procede. Comprobar modelo, unidades y export contra referencia; no cambiar tolerancia del modelo para mejorar FPS.
- **Editor lento al pintar:** probar preview adaptativo respetando perfil de color, capas y original. Revisar undo/redo y export final a resolución/calidad requerida; una miniatura visualmente similar no prueba fidelidad del archivo exportado.

Entregar evidencia de latencia y memoria junto con equivalencia/error medido y límites. Aplicar [release readiness](release-readiness.md) según los flujos del dominio: seleccionar controles pertinentes a sus resultados, sin añadir una lista universal ni prometer corrección global.
