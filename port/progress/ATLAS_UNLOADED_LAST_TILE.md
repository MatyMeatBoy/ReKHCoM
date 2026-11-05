# Atlas: referencias al último tile no cargado

El editor rechazaba las escenas 130, 076 y 117 antes de crear el proyecto.
Sus mapas ROM contienen respectivamente dos celdas `0x53ff`, una `0x73ff`
y una `0x33ff` en BG2; todos referencian el tile 1023, fuera del banco
cargado (261, 1023 y 242 tiles). No hay un segundo banco omitido para BG2.

El motor **no interpreta 1023 como un sentinel especial**.
`src/engine.c:LoadBgTiles` y `LoadBgTilesLz77` copian sólo los datos
proporcionados; `src/main.c:InitSystem/ClearSystemMemory` limpia VRAM.
`src/evt/mode_event.c:Event_0` sitúa estas capas en charbase 0 y sus mapas
en los bloques 21–23. `LoadEventBg3/LoadEventBg2Map/LoadEventBg1` en
`src/evt/event_message.c` no cargan datos adicionales para ese último tile.
Los objetos usan OBJ VRAM (`src/engine.c:LoadObjTiles`) y no son un banco BG
faltante. El estado caliente puede conservar otros bytes: no se afirma
paridad con una partida concreta ni un fotograma en ejecución.

Se representa exclusivamente la referencia ausente a 1023 como VRAM inicial
limpia, conservando tile/paleta/flips y celdas originales. La metadata dice
`initial-cleared-vram` y el editor muestra la limitación. El slot queda
excluido de los donantes del pincel y del cuentagotas; los datos ROM y su hash
no se rellenan. Un 1023 presente usa sus bytes reales. Otros índices ausentes
y un tile parcial siguen siendo errores. Guardado/exportación/importación
conservan los descriptores, incluso al mover una celda existente externamente.

Validación: 19 pruebas Python de Atlas, prueba Node de selección de donantes,
y prueba local de las tres escenas con ROM europea en sólo lectura. Esta
última reconstruye y compara todos los words de los mapas, verifica igualdad
de píxeles tras exportar/importar y persistencia de una celda movida tras
reiniciar. Evidencia privada: `pruebas/atlas-final-20261105/native-editor-fix/`.
No compilación, juego, navegador ni cambios en datos vivos de Atlas.
