# Galería: preview inmediata con respaldo GBA — 27/10/2026

Carriles `optiport-previews-both-modes-perf` y
`previews-both-modes-visual-fallback`, owner `codex-gallery-previews`.
Primer bloque acotado para revisión. No cerrar profiling/objetivo global por
estas pruebas sintéticas; dispositivo/GPU/p95 y revisión visual pendientes.

## Comportamiento

El menú real tiene GBA, HD y ReKHCoM híbrido. La fuente se elige por componente;
no confundir modos gráficos con pestañas ni eliminar HD puro. En los tres se
mantienen proporciones del poster y la resolución disponible. Una selección
HD sin captura inmediata muestra su poster GBA limpio o, si falta, el fondo
ROM del mismo evento. El título identifica GBA mientras se usa ese respaldo.
El worker HD existente sustituye el respaldo cuando termina; error/cancel o
medio ilegible conserva GBA, sin bloquear entrada ni abrir una escena.

Se mantiene un único worker y su debounce75ms/descarte por serial. No se carga
vídeo completo ni se adelanta el juego para conseguir un poster. PNG sidecar
HD todavía se lee/decodifica en selección; no prometer latencia cero.

CacheGBA LRU: máximo3entradas/1MiB de píxeles, aparte del buffer seleccionado
640x360 y un buffer del worker HD. Clave evento de ROM; sólo almacena poster/
fondo GBA, nunca un HD EN/JP. Nuevo menú limpia cache para ver PNG actualizados;
shutdown final libera todo. Fallo de malloc pierde sólo cache, no la imagen
seleccionada ni lógica. Se comparten posters entre GBA y respaldo híbrido/HD.
No cachear múltiples capturas HD sin perfil/presupuesto. Las claves/rutas HD
y generaciones de335 se conservan al apilar este PR sobre esa dependencia.

Se retira el marco exterior gris de la tarjeta existente; fondo interior
blanco, barra y tipografía del menú permanecen. Respaldo usa arte propio de
ROM o posters existentes, sin imágenes nuevas externas. Es una propuesta UI
que debe revisar el coordinador con capturas reales, no una aprobación GPU.
El gris embebido dentro de una imagen fuente no se elimina arbitrariamente.

## Pruebas y próxima evidencia

`test_gallery_preview_fallback.py`: productor real, decoder simulado400ms,
primer retorno contiene posterGBA o fondoROM; HD reemplaza después; un worker,
stale descartado al cambiar selección/cerrar, decoderfallido conserva respaldo,
cachewarm no redecodifica, límiteLRU/memoria y mallocfallido recuperable.
`test_scene_poster_async.py`: regresión original coldclose/stale/oneworker;
artefactos ahora temporales en vez de pisar carpetaD: histórica. Se compilan
funciones reales con SDL, sin juego/ROM/driverGPU. Los milisegundos impresos
son de este harness, no p95 de Steam Deck ni rendimiento del juego.
SyntaxWindows y guestfreestanding de galería/UI pasan. Sin heavybuild/runtime.

Root ejecuta serial: GBA/HD/ReKHCoM, coldfirst/warmback, selection rápida,
corrupto/ausente, EN→JP de335, regresar de playback y pérdida de renderer.
Capturar tarjeta en1080p/x4: sin marco gris, ratio sin estirar, GBA nítido,
HD de buena calidad al terminar, labels correctas. Comparar memoria máxima,
latencia entrada/primerposter/HDfinal y p95/p99 con SHA/config/assets registrados.
CacheGBA≤1MiB no demuestra presupuesto GPU; selectedtexture sigue una sola.
No pausar historia con precarga, no generar escenas esperando segundos.
