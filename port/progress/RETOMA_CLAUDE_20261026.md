# Retoma de los pedidos que Claude no llegó a procesar — 26/10/2026

Fuente: mensajes del usuario posteriores al límite de sesión de Claude, sesión
`b6535195-3ee4-4426-8bf8-b2fdcdd1f67d`. El usuario delegó decisiones y pidió workers.
Se conserva todo WIP. El usuario autorizó explícitamente el 26/10 publicar la **2.0 stable** después de terminar
todos los pendientes, depurar en general y obtener resultados correctos en las comprobaciones. No volver a pedir
esa autorización; no publicar mientras haya fallos sin resolver.

## Plan y verificaciones

- [x] Recuperar el traspaso de main y los mensajes realmente pendientes; separar notas antiguas de bugs nuevos.
- [x] Diagnóstico Android: APK sin imagen US; evitar resultados falsos del arnés (#239, `065f10ba4`).
- [x] Empaquetado Linux desde cero incluye host64 + SDL + tres imágenes (#240, `0419243fb`).
- [x] Linux pantalla completa ya resuelto (#227); no duplicar.
- [x] Entregar en Atlas título HD con textos, hija `ed3cd58de3e54bde89c268abbc8356b1`, sin inventar aprobación.
- [x] Controles: restablecimiento de bindings, captura e input; Nintendo vuelve a automático (#241, `3ab48f7a2`).
- [x] Gatillos con histéresis y tooltip L2/R2 en logros; letra centrada en botón GBA (#243, `24a57d9a1`).
- [ ] Galería: escenas normales y HD reproducen desde título/partida, vuelven a lista/selección, miniatura tipo
  cartel separada del nombre; checkpoints y videos liberan recursos al acabar/cancelar/fallar.
- [x] Launcher: bandera japonesa en Undub; misma portada de Steam también como hero superior (#242, `df028860e`).
- [x] Presentación: Brillo solo en GBA original o con opción experimental; deshabilitar L/R a la vez que el botón.
- [x] Corregir texto fuera de cajas, centrar el aviso GBA, completar franja inferior HD y fundidos de Opciones.
- [x] Terminar infografía de estados rápidos que Claude dejó en WIP; texto claro sobre logros.
  Estas tres entregas van en #244; pruebas en `TITLE_MENU_CLOSURE_20261026.md`.
- [x] **Claude (#256, #259):** repaso de textos propios (Opciones, avisos, carteles, launcher): prolijo,
  profesional, lenguaje de juego claro y respetuoso, sin cambiar el sentido ni romper los anchos. PR propio.
- [ ] Integrar PR pequeños verificados; completar gates y paquete limpio desde el commit de cierre.

## Pedidos posteriores y cierre técnico

- [x] Título y Opciones: #244 integrado (`124799ea0`); detalle y capturas en `TITLE_MENU_CLOSURE_20261026.md`.
- [x] Todas las luces de menús HD: #253 integrado (`68d484429`), cinco muestras originales y pulso compartido.
  Título/Opciones sin HD idénticos al original; renderer verifica también color cálido, fundidos y liberación.
  Atlas hija `24bee72a8205435792c6192b3a8239a0`, sin modificar calificaciones humanas.
- [x] Desfase del ojo de primera pestaña: #249 (`1e8905b0d`), cuatro cuadros corregidos; guante, difuminado
  inferior y Moguri sin cambios de píxeles fuera del defecto. No se cambió ningún estado a «Revisión 0».
- [x] Validadores de eventos/audio/frameskip: #251 (`795cd757e`) rechaza evidencia incompleta o de un binario antiguo.
- [x] Input y capturas: #248 (`204367b2a`); 200 ciclos reales de abrir/cerrar y captura de 13 teclas/13 botones
  correctos en esa versión. La matriz final debe registrar su propio SHA.
- [x] Galería GBA/HD/ReKHCoM: #250 integrado (`5505dd49a`); regreso al menú y conservación de estado/partida,
  póster estático, catálogo regional, HD equivalente al gameplay y fallback GBA. Dos revisiones independientes,
  pilotos de estado/SRAM, final/salto/pausa y alias recuperado tras vídeo corrupto. Sigue pendiente la matriz final.
- [x] Manejador Windows #258: DLL del host fuera de la emulación del bus; accesos GBA y single-step conservados.
  Cambio real de preset Riku por SDL correcto; #252 mueve las acciones del arnés al límite de entrada seguro.
  Matriz de Opciones terminada: 218/218, sin BIOS/crash; alcance y SHA en evidencia externa.
- [x] Ajustes de la pre-final: #261 alinea la base de SELECT y la luz; #262 prepara PNG HD360p y API de carga;
  #264 precarga menú y conserva dos cachés UI acotadas; #265 incluye 199 pósteres GBA sin HUD, salida HD real
  y un único trabajo de preview asíncrono. Cerrar/reabrir no espera al decodificador.
- [x] Brick Launcher excluido por instrucción directa del usuario: #266 retira completamente #263 y su figura.
- [x] No iniciar la matriz de galería de 2952 casos: el usuario indicó que las pruebas eran suficientes.
  Se conservan los pilotos reales, auditorías de estado/SRAM y regresiones dirigidas; no se atribuye esa matriz como ejecutada.
- [ ] Generar paquetes desde el SHA unificado, comprobar contenido/arranque y gates de lanzamiento, publicar v2.0 estable.

El usuario reafirmó la publicación en todas las plataformas previstas, pidió responder los hilos pendientes y avisar
a quienes recibieron pretests. Autorizó limpiar nuestras respuestas previas y cerrar los issues resueltos. Al terminar
los lanzamientos de consola y las respuestas, detenerse; no abrir nuevas baterías ni seguimientos automáticos.

No atribuir al juego el último fallo de compilación: clang informó explícitamente falta de espacio en C:.
Se deduplicaron únicamente fixtures propios con hashes verificados; temporales de compilación pasan a D:.
Partidas y archivos del usuario se conservan.

## Decisiones

- Se retoma la funcionalidad existente y se mantienen sus preferencias; no se rediseña la estética.
- El brillo simula la pantalla física de GBA: en modos modernos queda neutro y sin atajo. La opción experimental
  explica su finalidad y vuelve a habilitar botón y atajos; modo GBA original conserva ambos.
- Previews de galería son capturas estáticas y caché acotada; no reproducir ni decodificar videos al pasar filas.
- Las pruebas previas pertenecen a su SHA. El runner antiguo se detuvo por indicación del usuario, conservando
  informes y capturas; se limpiaron binarios, objetos y copias de prueba. Las ROM y partidas originales se conservaron.
- Un APK incompleto es error de entrada, no evidencia de un combate colgado. Repetir el caso con app completa.
- El WIP de `v20-wt` se conserva intacto; cualquier rescate se hace como cambio explícito y probado en otra rama.
