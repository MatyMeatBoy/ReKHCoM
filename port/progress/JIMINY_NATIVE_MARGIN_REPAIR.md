# Márgenes del submenú interno, normal y HD

La referencia del usuario es el detalle de «Tres deseos», no la cubierta del
diario. El menú amplio normal no reservaba el pie ni exponía los colores del
nombre porque esas rutas dependían de los assets HD. La página derecha acababa
en el límite GBA y su borde sombreado se confundía con bandas extensibles.

La elegibilidad del pie y los metadatos del nombre ya no dependen de HD. Las
tres páginas de detalle capturan, durante la composición real, papel, reglas,
borde, fondo y propiedad BG1. Sólo se reparan píxeles del borde que pertenecen
a BG1 y que el compositor de UI no modificó. Se prolonga papel plano con sus
reglas; la guía derecha se traslada al borde nuevo. No se estiran columnas de
textura. La pestaña PAGE, retratos, textos, naipes y objetos conservan prioridad.

Los mapas nativos confirman que las columnas224..239 de las filas20..23 ya
son sombra de papel (índices136..142), no PAGE. Repararlas elimina los cuatro
renglones residuales que quedaban bajo la pestaña. Las bandas genéricas del
detalle se limitan a la cabecera, sin repetir fragmentos del marco inferior.
Original/CRT mantienen su disposición clásica.

Los colores conservan el LUT y los fundidos del PPU presentado; alfa no
reconstruible rechaza el reemplazo. Un paquete inválido, una ventana estrecha
o un fallo de dibujo conserva/restaura la imagen nativa. La pintura se agrupa
por bandas iguales, con prueba de menos de100 llamadas en el caso plano, sin
asignaciones ni texturas nuevas por fotograma. No es una medición de stutter.

Pruebas CPU: geometría con y sin bridge, getter normal RED en la base y GREEN
actual, propiedad del nombre, renderer real con fallos de dibujo y protección
de objetos, paleta/fundidos y roles de los tres mapas originales. Fuente base
c44f679; evidencia privada bajo pruebas/jiminy23-submenu-requested. Capturas
EU32 y EU64 normal/HD de Historia, Naipes y Personajes terminan0 (24 imágenes
de cuatro recorridos); la partida original se conserva idéntica. Software32 y
D3D11/bridge64 tienen diferencias de muestreo: no se afirma igualdad de píxeles
entre esos renderers. Esta validación no certifica audio ni hardware Android.

No modifica recursos importados, arte de portada ni lógica del juego. El
paquete público pre1 es anterior: este bloque necesita una entrega posterior.
