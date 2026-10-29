# Implementación apoyada en el motor

Aplicar cuando haya código decompilado, símbolos o conocimiento suficiente del motor para mejorar la implementación desde su causa. No limitarse a tapar síntomas si puede corregirse el contrato que falla; tampoco exigir conocer todo el motor antes de un arreglo acotado.

Reconstruir el tramo relevante del flujo: funciones exactas y callers, estados de entrada/salida, timing, memoria, ownership y formatos. Distinguir hechos del source/símbolos, observaciones del binario y suposiciones. Identificar quién produce, consume y libera el dato y qué estado es autoritativo. Preservar contratos originales de procesamiento, input, estado, direcciones y datos al escoger el punto de extensión del pipeline.

Si falta conocimiento, observar o instrumentar esa frontera y consultar reportes relacionados mediante [recuperación selectiva](progressive-retrieval.md); un issue cerrado o diagrama genérico no demuestra cómo funciona esta versión. Corregir la causa donde la evidencia la localiza. El fallback puede complementar esa corrección para recursos opcionales; no sustituirla por un catch amplio ni reanudar memoria corrupta.

## Esquemas mínimos, sólo donde correspondan

| Frontera | Contrato que debe quedar explícito |
| --- | --- |
| Recursos asíncronos | Request con identidad/versión → cancelación y decode → upload en hilo permitido → ready → consumo/cache → invalidación/release. Acotar colas y memoria; un archivo encontrado no implica recurso listo. Resultados obsoletos no deben reemplazar el estado actual. |
| Puente ABI | Separar direcciones/datos originales de 32 bits de punteros y llamadas host; adaptar la frontera según tipos y convención reales sin cambiar el formato del software. Ver [migración](architecture-migration.md). |
| Reset del renderer | Invalidar recursos de la generación anterior y recrear los necesarios con ownership claro. No publicar uploads tardíos contra un dispositivo/contexto ya reemplazado. |
| Entrada/salida de escena | Definir dueño del estado y UI, orden de activación/limpieza y cancelación. Una salida temprana o cambio de modo no debe activar después trabajo de la escena anterior. |
| IO y guardado | Tratar escritura, validación, sustitución y cambio del estado en memoria como transacción; conservar el estado anterior ante fallo. No considerar éxito sólo porque se escribió un temporal. |

Usar estos esquemas para comprobar invariantes, no para imponer una arquitectura nueva. Migrar incrementalmente en bloques verificables, conservando baseline y compatibilidad; evitar un rewrite grande motivado sólo por preferencia de diseño.

## Estado de objetos al reutilizar memoria

Si un objeto funciona al inicio pero falla tras recrearlo, seguir el allocator,
su inicializador y cada campo leído por el consumidor afectado. Una asignación
no implica memoria a cero; tampoco el funcionamiento de una sesión limpia
acredita su estado después de reutilizar un bloque. Reproducir con reutilización
real o patrones controlados sólo en el trabajo del objeto pertinente, incluyendo
un control limpio, y comprobar la transición real antes/después de inicializar
el campo según su contrato. Mantener las herramientas de inyección separadas
del paquete entregado y revisar los productores hermanos del mismo consumidor.

Preferir la corrección del inicializador a un clamp global, desactivar el
consumidor o limpiar indiscriminadamente todos los objetos. El mecanismo
reproducido bajo memoria alterada es evidencia acotada: conservar como pendiente
su atribución a un dispositivo cuando el flujo limpio aún no falla allí.

## De comprensión a evidencia

Reproducir el fallo en el producer/consumer real y fijar una aserción que distinga fallo de éxito. Incluir control válido, errores inyectados pertinentes, transición/cancelación y medición A/B si se pretende mejorar rendimiento. Probar la combinación final del código integrado, no sólo un modelo simplificado del pipeline.

Ejemplo de decisión: un recurso HD que falla el primer upload y deja un hueco requiere comprobar el punto donde se omite el dibujo original y condicionar esa omisión a readiness. Reproducir decode/upload fallido y recurso válido es más útil que rediseñar de entrada todo el grafo de recursos. Extender el cambio sólo si la evidencia muestra otro contrato roto.

Consultar el [catálogo de software](source-catalog.md) si aporta un mecanismo que falta; consultar sólo las fuentes pertinentes al patrón.

Si el producto adapta la presentación de un remake/remaster, verificar las [equivalencias de datos y representación](remaster-equivalence.md), especialmente capacidad, interpolación y grupos animados; compartir assets no demuestra compartir reglas.

