# Reproducciones y harnesses

Convertir el fallo del usuario en un recorrido verificable antes de optimizarlo. Un harness es una herramienta para ejecutar y observar ese contrato, no un sustituto automático del software ni del dispositivo.

Registrar región/revisión e identidad del contenido sin redistribuirlo, estado inicial o documento compatible, escena, secuencia de input con sus tiempos, assets/opciones y transición exacta. Asociar el resultado al SHA de fuentes, hash del binario, arquitectura y backend. Conservar comandos, logs y artefactos necesarios para repetirlo, evitando documentos o datos privados en entregas públicas.

Elegir un reloj explícito: pasos lógicos u operaciones para input determinista; reloj monotónico para latencia y timeouts. Distinguir pausa, falta de progreso y finalización. Si la reproducción necesita un estado que no se tiene, marcar esa limitación y ejecutar pruebas independientes; llegar a una pantalla inicial parecida no reproduce otra transición.

Definir una aserción observable de entrada y salida de la transición: escena alcanzada, consumidor ejecutado, recurso usado, estado conservado o frame presentado según el fallo. Un log de intención o input enviado no demuestra que la transición ocurrió. Una captura negra en modo headless no es evidencia visual del resultado; usar el renderer/captura apropiados o limitar la conclusión a lo que se observó.

Comprobar qué subsistema posee cada indicador antes de interpretar una discrepancia. Un resumen del documento, mapa o vista anterior puede permanecer intacto mientras otro modo trabaja correctamente. Seguir el productor del dato y observar el estado del consumidor activo con símbolos del binario exacto, una salida verificable o una captura adecuada. No corregir el motor sólo porque un indicador heredado parezca contradecir el escenario; registrar por separado ambos ámbitos.

Comparar el control negativo que exhibe el fallo con el positivo corregido, manteniendo escenario y condiciones. Para fallos inyectados, incluir también recurso válido y verificar el consumidor real, limpieza, límites de trabajo y fallback. Si el negativo no falla, el ensayo no demuestra esa corrección. Registrar repeticiones y resultados sin extrapolar a otras regiones, builds o dispositivos.

En pruebas de audio, exigir muestras de productor no nulas y la pista activa
prevista antes de evaluar una interrupción. Registrar recuperaciones además de
la cola: llenar con silencio sintetizado puede mantenerla positiva. Un ensayo
silencioso o dummy no acredita continuidad audible; aplicar [contratos de
audio](audio-stream-contracts.md) para separar transporte y escucha real.

En un ajuste visual localizado, confirmar la identidad del recurso desde el
consumidor, no sólo por su silueta o nombre supuesto. Aislar el reemplazo con un
recurso válido transparente cuando haga falta conservar su ruta de aceptación;
borrarlo podría activar otro fallback y confundir la causa. Medir el delta del
render real y su región, con controles de esquinas, paleta y arte que deben
permanecer. Un delta nulo tras pasar el fixture exige revisar la reproducción o
la identidad antes de declarar el defecto corregido.

Cuando hagan falta documentos o estados de prueba, seleccionar un corpus pequeño por cobertura distinta: región, formato, progreso y transición que desbloquea cada caso. Registrar URL/fuente si es pública, hash, procedencia y checkpoint esperado; descartar casos redundantes o con modificaciones o estado desconocido que confundan la causa. Conservar originales inmutables y leer/importar sólo hacia una copia scratch. No incluir contenido protegido ni publicar documentos privados del usuario; si no hay permiso de redistribución, conservar evidencia privada o crear un sustituto saneado. Un corpus amplio sin checkpoints verificables no sustituye buenas reproducciones.

## Validar el observador y cerrar la sesión

Antes de atribuir un fallo al producto, verificar el observador: paquete/proceso/Activity o ventana correctos, PID y generación vigentes, permisos/canal de captura y postcondición verificable. Una captura/XML vacío o inválido, un `input` enviado o un timeout del script no demuestran crash, ANR ni transición completada. Acotar los reintentos de adquisición sin ocultar el primer fallo; conservar salida, timestamps y, si existen, trazas/estado del proceso para distinguir fallo del harness, timeout sin causa y fallo del producto. Un proceso fresco puede ser el control pertinente para separar estado heredado de recreación; no borrar datos para obtenerlo ni convertir el reinicio en una recuperación que el usuario no tiene.

Si varias pruebas usan un dispositivo, emulador, puerto o canal de control compartido, dar un solo dueño a esa sesión. Al cancelar o entregar el recurso, detener y recoger sólo los hijos propios, terminar lectores/escritores pendientes y verificar quiescencia antes del siguiente controlador; un mensaje de timeout no acredita que el hijo terminó. Registrar PID/handle, último checkpoint, recursos aún vivos y condición de handoff. No competir con otro ADB/controlador ni matar procesos del usuario para liberar la sesión. Probar el cierre temprano y la cancelación durante una operación bloqueada según el riesgo.

Recoger sólo evidencia que decida la hipótesis: sanitizar rutas/identificadores y mantener estados, documentos y snapshots privados fuera de paquetes públicos. Un diagnóstico opcional debe declarar qué recoge, activación y límites; comprobar sus bytes/entrada reales y su desactivación, no asumir que una captura o build de diagnóstico reproduce el paquete normal. La ausencia de una traza necesaria se registra como límite, sin sustituirla por datos de otro proceso o prometer una causa.
