# Oogie: conexión de máscara y silueta del fantasma, sin build

Seguimiento de Atlas `85b1e5551e224f3893cf44fcc9cf5739`, revisión3 pending,
SHA256 `83ae33d489032531cad935890e63d4ffa92597e922b9ace6bb93252ca6104619`,
envío `20261105-203951-55f838`. Dos nubes: máscara
`62da4ae1-496f-4473-bf0c-32522821b471` y fantasma
`9afefae1-ae97-4699-a768-ed8531500807`.

La propuesta PR606, fuente `5bc97737699c1962e769d1e81350cc3cb32aa180`,
es la base de esta rama y se conserva como vista previa `detail`.
La hija sigue pendiente de valoración y se prueba con
`KHCOM_OOGIE_PREVIEW=shapes`. La arena aprobada del PR593 es la predeterminada.

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
El fixture C compara el renderizador con las tres fuentes exactas y verifica
la franja, el cuarto nativo y la paleta sobre datos sintéticos.

Comparativa antes/propuesta: simulación selectiva sobre la imagen anterior
de PR606, basada a su vez en la captura histórica300, escala3. Los512píxeles
mundo visibles modificados coinciden antes exactamente con el modelo
anterior:4608píxeles físicos. Las30diferencias restantes de la máscara están
ocultas por camino/ribete y se conservan. El cuarto entero permanece idéntico.
Los recortes se amplían a escala entera para revisar ambas figuras.

Faltan valoración humana y QA en el juego de oclusión, cámara, Original y
plataformas. Las capturas de referencia continúan siendo simulaciones.
Evidencia privada: `pruebas/atlas-20261106/oogie-shapes/`.
