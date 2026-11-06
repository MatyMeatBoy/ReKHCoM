# Oogie: conexión de máscara y silueta del fantasma, sin build

Seguimiento de Atlas `85b1e5551e224f3893cf44fcc9cf5739`, revisión3 pending,
SHA256 `83ae33d489032531cad935890e63d4ffa92597e922b9ace6bb93252ca6104619`,
envío `20261105-203951-55f838`. Dos nubes: máscara
`62da4ae1-496f-4473-bf0c-32522821b471` y fantasma
`9afefae1-ae97-4699-a768-ed8531500807`.

La propuesta PR606, fuente `5bc97737699c1962e769d1e81350cc3cb32aa180`,
es la base de esta rama. Se conserva íntegra como propuesta pendiente; esta
hija tampoco hereda aceptación y no autoriza por sí sola integrar606.

La mandíbula exterior terminaba en x255 mientras su continuación inferior
empezaba en257: el píxel256,472 sólo tenía la rampa verde. Se conecta en negro
y se recoge el lóbulo que sobresalía hasta288. El fantasma recibe un brazo
corto con punta definida y una falda derecha escalonada unida al cuerpo.
Sólo cambian polígonos y su propia máscara indexada empaquetada, sin espejo,
arte IA, colores RGB fijos ni píxeles de cartucho copiados al repositorio.

Se mantienen los índices activos146/157/158. `port_oogie.c`, PPU, guardas de
Original y cuarto nativo, `PathIndex`, franja frontal y planos verdes son
idénticos a la base. El generador sólo actualiza su header de máscaras propio;
no se ejecuta el importador ni se regeneran imports globales.

Pruebas puras Python: cinco unittest PASS, incluyendo dos fallos previos de
unión/silueta;24320coordenadas de máscara,15360píxeles internos excluidos y
542entradas cambiadas. El header coincide exactamente con su generador.
No se ejecutan los fixtures C de arenas ni pruebas que invoquen compilador.

Comparativa antes/propuesta: simulación selectiva sobre la imagen anterior
de PR606, basada a su vez en la captura histórica300, escala3. Los512píxeles
mundo visibles modificados coinciden antes exactamente con el modelo
anterior:4608píxeles físicos. Las30diferencias restantes de la máscara están
ocultas por camino/ribete y se conservan. El cuarto entero permanece idéntico.
Los recortes se amplían a escala entera para revisar ambas figuras.

**SIN BUILD:** no es nueva ejecución ni captura del juego. Faltan valoración
humana y QA real de oclusión, cámara, Original y plataformas cuando se
autoricen builds. No compilador/juego, imports, paquetes, releases ni cambios
en ROMs, partidas o instalación. Referencias ROM sólo internas.
Evidencia privada: `pruebas/atlas-20261106/oogie-shapes/`.
