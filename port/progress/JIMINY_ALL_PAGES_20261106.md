# Pepito: bordes y márgenes en todas las páginas

La reparación aceptada del detalle no se aplicaba a la portada ni a las listas.
Las filas del papel sólo se capturaban para los tres mapas de detalle y el
presentador sólo invocaba el arreglo en style12. La extensión genérica de
barras podía repintar después las franjas inferiores de las listas.

El mismo arreglo se aplica ahora a los ocho mapas estables: las portadas de
Sora/Riku, las listas de historia/naipes/personajes y sus tres detalles.
Se comprueba el cuerpo del mapa realmente presentado, conservando el fallback
de apertura/cierre y mapas desconocidos. La portada mantiene su cubierta azul,
las listas su cubierta izquierda/Pepito y sus reglas, y el detalle su guía
vertical. Cada tipo usa sus propios colores y altura inicial de curva.

El fondo izquierdo se registra desde BG1 para listas/portadas; así se extienden
la cubierta y su franja inferior sin convertirlas al verde del detalle. Sólo
el detalle conserva la limpieza de las tres columnas nativas de la izquierda.
La reparación derecha sigue limitada al ganador BG1 opaco, sin mezcla y sin
ediciones posteriores de UI. Textos, objetos, espiral y cabeceras conservan su
propiedad. El perfil usa el byte19 de padding; la fila sigue midiendo20bytes.

La ayuda de14filas continúa a todo el ancho, tanto con HUD HD como sin él.
Su fallback de fuente nativa y el ajuste240x160+14 no cambian. Original/CRT
conservan la exclusión del ajuste y de esta reparación. No se modifican las
recetas/imports ni las piezas de arte aprobadas.

Comprobaciones sin build:128combinaciones de selección interpretadas desde C,
32casos de captura/renderer con donantes de los ocho mapas ROM y cuatro tamaños,
seis fixtures que conservan exactamente los comandos del detalle aprobado,
ABI20/offset19, separación de cabecera/pie y conservación del fondo izquierdo.
También se revisan máscaras de foreground y restauración tras error de dibujo.
El fixture C de margen se actualiza pero no se compila ni ejecuta.

No hay captura nueva del juego ni validación SDL/float32/GPU o de plataformas.
La comprobación visual conjunta queda para la futura build autorizada.
Evidencia privada: `pruebas/atlas-20261106/journal-all-modes/`.
