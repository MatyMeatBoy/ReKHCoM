# Capas de verificación

Aplicar según riesgo y plataforma, con criterios explícitos de salida. No exigir infraestructura de estudio a un proyecto pequeño ni generar tests que sólo copian la implementación.

| Capa | Qué comprobar |
| --- | --- |
| 1. Diagnóstico | Binario exacto y símbolos, log retenido, configuración y transición reproducible; separar hechos de hipótesis. |
| 2. Correctitud | Warnings pertinentes, análisis estático y sanitizers disponibles; bounds, overflow, alineación, punteros y ownership. Una excepción capturada no hace segura memoria corrupta. |
| 3. Fallos controlados | Inyectar errores en asignaciones, IO, decoder, upload y colas. Verificar salida acotada, limpieza y software original aún disponible. |
| 4. Regresión funcional | Ruta base y recursos opcionales, opciones por separado y juntas, cambios de escena y flujo real de input. Captura/log de intención no prueba que se dibujó un frame. |
| 5. Concurrencia y ciclo de vida | Afinidad de hilo, carreras, resultados obsoletos, cancelación y shutdown; foco, suspensión, audio/dispositivo perdido y reanudación. Los timeouts no deben contar una pausa como falta de progreso. |
| 6. Rendimiento | Escenarios iguales, varias muestras, cold/warm, p95/p99 y picos; presupuesto CPU/GPU/IO por etapa. Comparar calidad y latencia además de FPS medio. |
| 7. Resistencia | Recorridos largos y repetidos para detectar crecimiento de memoria, handles o colas. Condiciones limitadas de RAM/IO y matriz de contenido, sin dañar la instalación real. |
| 8. Plataforma | Windows: ABI, foco y audio/GPU; Linux: ABI, librerías, permisos y runtime de distribución; Android: lifecycle, memoria y GPU/codec; web: límites de memoria, contexto gráfico y política de audio. Adaptar macOS y otros sistemas a sus backends. |
| 9. Entrega | Build desde SHA registrado, dependencias y hashes, contenido permitido, prueba de arranque y recorrido afectado del paquete final, instalación aislada y rollback. Confirmación del reportero por separado. |

## Términos útiles para decidir

- **Backpressure:** limitar al productor cuando el consumidor no alcanza; no acumular memoria sin límite.
- **Readiness:** recurso realmente utilizable, no simplemente encontrado en disco.
- **Frame pacing:** regularidad entre cuadros; un promedio alto puede ocultar tirones.
- **Degradación controlada:** reducir calidad o frecuencia de presentación conservando resultados e input.
- **Guardado atómico:** escribir y validar un archivo completo antes de reemplazar el anterior; comprobar errores y límites del filesystem.
- **Soak test:** recorrido prolongado para encontrar acumulaciones; no sustituye una reproducción dirigida.

Registrar herramientas y controles realmente ejecutados. Un emulador, renderer software o sanitizer es evidencia complementaria, no certificación de hardware ni dispositivos inaccesibles.
