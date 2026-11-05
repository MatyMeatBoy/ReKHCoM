# Agrabah107/108: continuación superior y último escalón, estudio sin build

Padre Atlas `7e56559f87b1402da2fe92e2eafe256c`, revisión4, SHA256
`6e67d42b2eb23e94b0a4707e3a3244c329a9ccb8a7a727cd73deca918a65cffa`.
Tres nubes: terminar una diagonal, completar ventana y llevar el patrón del
muro hacia arriba. La propuesta anterior entró en PR599; esta hija es nueva.

El helper ya completaba dos pilastras y la ventana cortada superior con sus
donantes originales, pero el PPU lo condicionaba al flag de preview global.
Activarlo en la ruta normal exclusiva de Agrabah permite continuar esas
columnas arriba y completar la ventana desde ocho columnas a la izquierda y
cuatro filas más abajo. No cambia su algoritmo ni duplica otra ventana.

El último escalón BG2 ocupa x496..511/y288..303: usa la misma traslación
(-32,-16) del otro remate ya integrado. Sólo cambia BG2 y sus77píxeles de
color distintos dentro del rectángulo; BG3, techo amarillo y calle naranja
se conservan. El gate de corridas incluye el rectángulo nuevo para no saltar
la reparación en el camino rápido. Todo se aplica fuera del cuadro GBA.

Verificación sin build: dos regresiones RED→GREEN con el AST C y el flag del
caller real.36.864coordenadas del selector de tiles,4.608píxeles cercanos al
remate y4.608corridas coinciden con el modelo. Cuatro recortes del antes
(diagonal, ambas pilastras y ventana) coinciden exactamente con la captura
histórica108/300 a3x, origen(627,63), después de expansión RGB555.
El rectángulo que cubre el área GBA de esa cámara permanece igual.

La imagen nueva es una **simulación sin build** del mismo espacio/escala.
La franja superior más alta muestra una continuación propuesta, no otra
cámara ejecutada. Se amplió el fixture C pero no se ejecutó. No compilador,
DLL, juego, importación ni paquete. Falta runtime nuevo, otras cámaras,
modo Original ejecutado, plataformas y valoración propia de esta hija.
Evidencia privada: `pruebas/atlas-siguiente-20261105/scene107/`.
