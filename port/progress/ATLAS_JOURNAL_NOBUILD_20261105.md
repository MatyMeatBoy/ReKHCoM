# Diario: estudio de footer, propiedad y esquina, sin build

Padre Atlas `a36793f6231a4c55b3ca636305aea2b7`, revisión5, SHA256
`338b9c20de1243e742211e4d9c372253cb74752c58ba1fe0b3afc7e749323f34`.
Las cuatro nubes piden acabar ambos extremos del footer, redondear la esquina
de la página y limpiar la línea negra exterior con su verde de fondo.

El WIP anterior de `atlas-journal-final` se respaldó con diff binario y cinco
hashes antes de trasladarlo por parche3way a este checkout nuevo. El original
permanece intacto. Se conserva la fuente HD posterior909PNG+2LAD en sólo lectura;
no se reimportó ni copió ningún asset a una instalación personal.

Propuesta: capturar la propiedad BG1 opaca en las tres columnas exteriores
izquierdas. Sólo sus píxeles ganadores y sin edición posterior dan paso al
verde presentado. El borde derecho usa contornos circulares de radios4/3,
conserva colores/reglas de la fila presentada y agrupa rectángulos planos.
Objetos, BG0, ventanas, blends y modificaciones del compositor se conservan.
`leftOwned` usa un byte del padding existente: fila20bytes, offset18.

Se corrigió además un defecto del WIP a escala3.5x: `floor(gy)` adelantaba una
regla un píxel en empates de medio píxel. El índice `ceil(gy)-1` invierte
`PixelEdge` para el color, manteniendo el centro continuo para la curva.

Verificación permitida: interpretación del AST del renderer real con grabador
de comandos SDL,13.536muestras de fila,2.016casos de propiedad BG1, ABI20 y
11rechazos de dibujo que invocan restauración. Fixture plano con una regla y
máscaras de control:39..74rectángulos en escalas1.5/2.5/3.5/4/6, menos de100.
Regresión fraccional RED→GREEN. Los cinco hashes del WIP original se conservan.
Los fixtures C fueron actualizados/trasladados, **sin ejecutarlos**.

La imagen es un **estudio geométrico con propiedad BG1 controlada**, no una
captura del juego ni un volcado de propiedad del frame humano. Los colores
mostrados están presentes en la imagen de Atlas; los cuatro pares ilustran
las propuestas del código, no la resolución comprobada de sus nubes en juego.
Faltan máscaras reales, SDL/float32, navegación/lista/detalle/vuelta, modo
Original y plataformas. No compilador, DLL, juego, imports ni paquete.
Evidencia privada: `pruebas/atlas-siguiente-20261105/journal/`.
