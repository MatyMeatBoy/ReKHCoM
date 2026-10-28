# Pasada completa de OptiPort sobre ReKHCoM 2.2

Reanudación del usuario, 28/10/2026: resolver [441](https://github.com/MatyMeatBoy/ReKHCoM/issues/441), [439](https://github.com/MatyMeatBoy/ReKHCoM/issues/439), revisar OptiPort completo con los issues aprendidos y evaluar cobertura adicional. Base de código874b8942; método público433fb645. Los paquetes2.2 publicados son inmutables. El trabajo posterior en main no demuestra que sus bytes estén en esos paquetes.

## Alcance y avance

La batería histórica conserva **10/14 bloques integrados (~71%)**, con los contratos originales de [la primera pasada](OPTIPORT_V21.md). La auditoría de24 documentos de la skill mide cobertura documental. Ninguna de esas cifras es un porcentaje de estabilidad ni de esta pasada ampliada.

Se evalúan **16 áreas**: las14 originales y dos contratos que antes estaban fuera del denominador. El porcentaje global nuevo queda **pendiente de conciliación**: primero se contrasta cada criterio ampliado con la evidencia vigente. No se trasladan automáticamente diez cierres a un denominador mayor, ni se cuentan lectura, implementación, pruebas y confirmación externa como si fueran el mismo estado. Se reutiliza evidencia válida para el mismo contrato, binario y calidad; se repiten sólo las comprobaciones afectadas.

## Matriz de evaluación

| Área | Evidencia disponible / siguiente criterio de cierre |
| --- | --- |
| 1. Logs y diagnóstico | #282/#298 y #444 integrados. Fallos de arranque conservan etapa/error antes de limpiar; primera textura o salida0 no prueban un flujo terminado. Contrastar el paquete que se entregue y no confundir Activity finalizada con proceso muerto. |
| 2. Portabilidad y alineación de datos | #286/#318/#319; repetir el caso real de puerta/guardado si cambia el productor pertinente. La alineación de ROM/BIOS y el formato persistido conservan su contrato original. |
| 3. Pools y paletas | #285/#286; reutilizar agotamiento inyectado y retorno al recurso original donde el consumidor permanezca idéntico. |
| 4. Imágenes y presupuestos | #284/#296; validar cabecera antes de decode, solapamiento de buffers, recursos activos fijados y límites ante falta de memoria. No contar enlaces duros como memoria o almacenamiento adicional. |
| 5. Readiness y composición HUD | #283/#287/#297 cubren recursos e invalidación. #441 añade comparación Linux/Windows del texto y la barra: productor, máscara, orden y presentación, con cambios confinados a la región; no basta un helper geométrico verde. |
| 6. Música HD | #288/#291/#295 y contratos de audio posteriores. Medir frecuencia de entrada, cola y consumo de pista por separado; continuidad requiere señal no nula y prueba de recuperación, no sólo una cola positiva. |
| 7. Vídeo HD | #289/#293/#421 aportan readiness, controles y lectura fragmentada. #439 requiere además observar cuadros distintos, tiempo de decode/transporte/upload y presentación conservando la misma calidad. |
| 8. Profiling | Parcial histórico. Comparar fuentes, assets, configuración y caché equivalentes; conservar muestras, dispersión y coste del observador. Una cohorte a menor resolución explica coste, pero no acredita una mejora a igual calidad. |
| 9. Asíncrono y precarga | Parcial histórico. Medir entrada/cache/latencia y verificar cancelación y generaciones; un resultado tardío no debe restaurar una escena o recurso obsoleto. |
| 10. Streaming y backpressure | #291/#293 y controles de progreso. Para #439 comprobar bytes, fragmentos, esperas vacías, EOF, cancelación y cola acotada; eliminar una espera no autoriza polling sin límite. |
| 11. Frame pacing | Parcial histórico. Separar FPS del juego, frecuencia del vídeo, cuadros nuevos y presentaciones repetidas; medir cadencia y colas sin cambiar el reloj GBA. |
| 12. Lifecycle | #309/#311 y diagnóstico posterior. Reproducción mínima de foco/pausa/recreación con PID y contexto correctos; un fix de configuración sin confirmar no cierra S22/Moto ni se integra por haber cerrado un PR. |
| 13. Protección del progreso | #290/#314 y regresiones de guardado. Fallos de escritura/reemplazo conservan estado previo; probar cancelación y espacio insuficiente en copias aisladas. |
| 14. Entrega y resistencia | Parcial histórico. Artefacto exacto, procedencia por componente, escenarios representativos y límites por plataforma. Los reports de dispositivo sin causa/confirmación siguen separados de las regresiones locales. |
| 15. Contratos entre runtimes y paquetes — ampliación | Matriz explícita host/guest, ABI/capacidades, aritmética, cachés y dependencias. Conservar los resultados GBA y errores originales a través de las arquitecturas anunciadas. #435/#438 aportan pilotos; #440 cerrado sin merge no acredita ARM32. Exigir actualización de productores cacheados y prueba del paquete exacto para cada alcance prometido. |
| 16. Fiabilidad del ensayo y coordinación — ampliación | Identificar fuente/binario/PID/ventana/recurso observado y postcondición. Conservar rawXML, capturas y salida incluso si falla el parser; distinguir ANR del sistema, timeout de entorno y fallo del producto. Handoff, cancelación y cierre dejan el recurso compartido en estado comprobado, sin matar apps ajenas ni perder el diagnóstico al apagar una VM prematuramente. |

Las áreas15/16 tienen criterios propios: no duplican la alineación de datos del área2 ni los logs del producto del área1. Su cierre requiere ensayos del contrato; añadirlas al documento no las completa.

## Hechos nuevos y pruebas decisivas

- **439:** el reportero confirma juego a60FPS e importación «Just the basics». Esa selección de contenido no acredita resolución menor. Medir la ruta Linux con el mismo archivo/frecuencia/resolución y diferenciar decodificación, transporte, cola, reloj y presentación. El backend Windows no certifica Steam Deck; las pruebas en VM no son confirmación física del dispositivo.
- **441:** barra roja tapa parte de «Start» en Linux2.2; el reportero no lo observa en Windows. Comparar el render final y sus capas con fuente, idioma y escala equivalentes antes de cambiar arte o recortar texto.
- **Importador:** el código actual describe la retirada del preset720p: los presets actuales no reducen resolución. Cada backend debe comprobarse, pues la ruta puede recodificar o copiar la fuente. Separar contenido importado, resolución, compresión/bitrate y cadencia. Verificar la salida real y la identidad de receta/caché; no deducirlo del nombre del preset. La reparación opcional de instalaciones existentes debe preparar/validar una salida antes de reemplazarla y respetar generaciones de media inmutables.
- **Arranque Android:** ausencia de `ApplicationExitInfo` no descarta retorno nativo con Activity finalizada; tampoco demuestra éxito. Empaquetado/AAPT, instalación y ciclo de vida son comprobaciones diferentes. Las limitaciones de los tests412 siguen publicadas; no se reinterpretan como fixes confirmados.

## Orden y entrega

OptiPort: revisión completa, cambios mínimos en referencias donde cambien decisiones, comprobación de links y casos de presión, PR e instalación sincronizada con copia previa y preservación del contenido local útil. Los nuevos fallos se promueven a lecciones verificadas sólo cuando haya mecanismo/evidencia; las hipótesis quedan en el proyecto.

Producto: carriles separados439/441 e importador/reparación, claims en la cola remota, PR pequeños y una entrega Linux de prueba que incluya sólo lo integrado/verificado. No sustituir estable2.2 ni sus prereleases. No reducir resolución de archivos existentes silenciosamente ni borrar originales, guardados o evidencia para obtener una cifra verde.
