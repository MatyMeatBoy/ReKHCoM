# Reproducciones y harnesses

Convertir el fallo del usuario en un recorrido verificable antes de optimizarlo. Un harness es una herramienta para ejecutar y observar ese contrato, no un sustituto automático del software ni del dispositivo.

Registrar región/revisión e identidad del contenido sin redistribuirlo, estado inicial o documento compatible, escena, secuencia de input con sus tiempos, assets/opciones y transición exacta. Asociar el resultado al SHA de fuentes, hash del binario, arquitectura y backend. Conservar comandos, logs y artefactos necesarios para repetirlo, evitando documentos o datos privados en entregas públicas.

Elegir un reloj explícito: pasos lógicos u operaciones para input determinista; reloj monotónico para latencia y timeouts. Distinguir pausa, falta de progreso y finalización. Si la reproducción necesita un estado que no se tiene, marcar esa limitación y ejecutar pruebas independientes; llegar a una pantalla inicial parecida no reproduce otra transición.

Definir una aserción observable de entrada y salida de la transición: escena alcanzada, consumidor ejecutado, recurso usado, estado conservado o frame presentado según el fallo. Un log de intención o input enviado no demuestra que la transición ocurrió. Una captura negra en modo headless no es evidencia visual del resultado; usar el renderer/captura apropiados o limitar la conclusión a lo que se observó.

Comparar el control negativo que exhibe el fallo con el positivo corregido, manteniendo escenario y condiciones. Para fallos inyectados, incluir también recurso válido y verificar el consumidor real, limpieza, límites de trabajo y fallback. Si el negativo no falla, el ensayo no demuestra esa corrección. Registrar repeticiones y resultados sin extrapolar a otras regiones, builds o dispositivos.

Cuando hagan falta documentos o estados de prueba, seleccionar un corpus pequeño por cobertura distinta: región, formato, progreso y transición que desbloquea cada caso. Registrar URL/fuente si es pública, hash, procedencia y checkpoint esperado; descartar casos redundantes o con modificaciones o estado desconocido que confundan la causa. Conservar originales inmutables y leer/importar sólo hacia una copia scratch. No incluir contenido protegido ni publicar documentos privados del usuario; si no hay permiso de redistribución, conservar evidencia privada o crear un sustituto saneado. Un corpus amplio sin checkpoints verificables no sustituye buenas reproducciones.
