# Diario interno: trabajo2.3 en curso

Primer bloque: registrar los seis dígitos de página con el emisor decimal
compartido. La ruta anterior hacía DMA directo y no proporcionaba significado
al texto HD. Se mantienen las seis copias y el orden; la rama de cartucho
conserva su código anterior. El índice, total, navegación y guardados no cambian.

`test_jiminy_page_digits.py` ejercita el fragmento real del diario y el emisor
real: falla sin los registros HD y pasa999 parejas de índice/total con las
seis copias nativas idénticas. Windows32 y64 recompilados; contratos ABI/boot64
pasan. Capturas reales de raíz, categorías, ataque, ThreeWishes, siguiente
entrada y retorno en32software y64Direct3D11, también en Auto amplio.

Evidencia privada: `pruebas/jiminy23`, `jiminy23-digits`, `jiminy23-wide`.
Los perfiles emplean una copia de partida y desbloqueo del diario sólo en RAM.
`view_mode=0` es Original, no Auto: la prueba amplia debe usar4. Las bandas
negras del primer perfil eran su configuración, no un fallo del diario.

**Pendiente antes de cerrar P2:** cabeceras internas/PAGE, limpieza del libro,
borde derecho redondeado, variantes de páginas y localización. Este bloque
no certifica esos elementos ni otros dispositivos, y no está en los paquetes
2.2.5 existentes. Continuar el diario antes de pasar al audio.

Segundo bloque: cabecera interna de naipes, que vive en BG1 y no en los
sprites del título principal. Se añaden cinco recetas independientes (inglés,
francés, español, italiano y alemán), con geometría y colores leídos del mapa
original y letras remasterizadas. La selección usa los tiles reales del idioma;
no reemplaza la cabecera JOURNAL/DIARIO de la raíz. Sigue el fundido negro/blanco.
El fondo original se conserva para HD apagado o recurso ausente.

Windows32 y64 compilados, ABI/boot y check_sources pasan. Capturas privadas
`pruebas/jiminy23-header` repiten la ruta amplia Auto4 de seis vistas por
arquitectura. Continúan pendientes PAGE, otras cabeceras y borde del libro.
Este bloque es parcial y está incluido en2.3-pre1 junto al contador de página.

Tercer bloque: ayuda inferior del diario con las acciones reales de abrir,
volver, cambiar entrada y cerrar. Conserva los 240×160 píxeles del juego y
reserva catorce filas para la ayuda, con una transformación común para todas
las capas. Incluye seis idiomas y etiquetas de los controles configurados.
Si falta la fuente o su textura no está lista, usa la fuente nativa sin crear
otra textura. Original, CRT y HUD HD apagado conservan la presentación previa.

El fundido usa la paleta capturada del fotograma presentado: después de
VBlank la paleta viva puede pertenecer ya al siguiente fotograma. Una captura
durante la transición reproducía ayuda negra sobre el libro visible; el
control corregido limita las diferencias al footer. La prueba CPU ejercita
las funciones reales con mutación de la paleta siguiente, negro, blanco y
forced blank. La geometría cubre salidas pequeñas, amplias, altas y no enteras;
la disponibilidad de glifos cubre ausencia, espacio y rechazo del bridge.

Evidencia privada: `pruebas/jiminy23-footer-snapshot32`, `snapshot64`,
`snapshotFractional64`, `snapshotFourthree64` y `snapshotOff32` (mismo prefijo).
Cinco recorridos reales terminaron con código 0 y cinco capturas cada uno;
el control HD apagado coincide píxel por píxel con las cinco vistas anteriores.
No se modificó la partida original. Esto no termina la página derecha, PAGE,
otras cabeceras, las extensiones ni el estiramiento de píxeles del libro.
La pre2.3-pre1 publicada permanece inmutable y no contiene este tercer bloque.

Cuarto bloque: las extensiones de cabecera, bandas y pestañas conservan sus
colores y posición junto al fotograma presentado. Antes, sólo la pestaña BG
se capturaba antes de VBlank; el resto podía leer la paleta/VRAM del siguiente
fotograma o descriptores de objetos ya liberados. La captura nueva usa un
buffer estático menor de40 KiB, limitado por las coordenadas OAM originales,
sin asignación por ventana y sin alterar la lógica ni el framebuffer nativo.

`test_menu_presented_margins.py --baseline-ref d0180e699` falla tras mutar los
datos del siguiente fotograma; la versión nueva conserva cabecera móvil,
bandas y pestañas, y borra la captura al presentar una imagen sin ellas.
Comprueba límites y todos los tamaños/formas OAM a lo largo de512 posiciones.
Las26 capturas reales del diario (32 y64) terminan con código0. En las diez
vistas comparables, sólo cambia el rectángulo de la cabecera izquierda que
antes quedaba negro durante la transición1721; el resto es idéntico.

Evidencia privada: `pruebas/jiminy23-margin32`, `jiminy23-margin64` y
`jiminy23-pages/MARGIN-PIXEL-SCOPE.json`. Una prueba adicional abre el mapa
mediante START/dirección/A desde una sala inicializada y termina con cuatro
capturas y código0 en64. La cadena artificial que forzaba cinco modos sin
preparar el mapa falla también en el binario anterior, en el mismo acceso
nulo de AllmapSetBounds; se conserva como control inválido, no como una
regresión nueva ni como validación de los menús posteriores al fallo.
Ver `jiminy23-pages/MARGIN-HARNESS-CRASH.json`. Siguen pendientes el libro,
PAGE y las demás cabeceras; este bloque tampoco cierra P2.
Las cuatro vistas del mapa abierto con controles normales coinciden con el
binario anterior. Otra prueba64 prepara una sala válida y abre la entrada
de la tienda Moguri, con cuatro capturas y salida0; sólo cubre su presentación,
no el audio ni todos sus submenús. Windows32/64 y las bibliotecas Android
US/JP están compilados; los contratos ABI/boot64 y check_sources pasan.
