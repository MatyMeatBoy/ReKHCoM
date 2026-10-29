# Diario interno: trabajo2.3 en curso

Primer bloque: registrar los seis d√≠gitos de p√°gina con el emisor decimal
compartido. La ruta anterior hac√≠a DMA directo y no proporcionaba significado
al texto HD. Se mantienen las seis copias y el orden; la rama de cartucho
conserva su c√≥digo anterior. El √≠ndice, total, navegaci√≥n y guardados no cambian.

`test_jiminy_page_digits.py` ejercita el fragmento real del diario y el emisor
real: falla sin los registros HD y pasa999 parejas de √≠ndice/total con las
seis copias nativas id√©nticas. Windows32 y64 recompilados; contratos ABI/boot64
pasan. Capturas reales de ra√≠z, categor√≠as, ataque, ThreeWishes, siguiente
entrada y retorno en32software y64Direct3D11, tambi√©n en Auto amplio.

Evidencia privada: `pruebas/jiminy23`, `jiminy23-digits`, `jiminy23-wide`.
Los perfiles emplean una copia de partida y desbloqueo del diario s√≥lo en RAM.
`view_mode=0` es Original, no Auto: la prueba amplia debe usar4. Las bandas
negras del primer perfil eran su configuraci√≥n, no un fallo del diario.

**Pendiente antes de cerrar P2:** cabeceras internas/PAGE, limpieza del libro,
borde derecho redondeado, variantes de p√°ginas y localizaci√≥n. Este bloque
no certifica esos elementos ni otros dispositivos, y no est√° en los paquetes
2.2.5 existentes. Continuar el diario antes de pasar al audio.

Segundo bloque: cabecera interna de naipes, que vive en BG1 y no en los
sprites del t√≠tulo principal. Se a√±aden cinco recetas independientes (ingl√©s,
franc√©s, espa√±ol, italiano y alem√°n), con geometr√≠a y colores le√≠dos del mapa
original y letras remasterizadas. La selecci√≥n usa los tiles reales del idioma;
no reemplaza la cabecera JOURNAL/DIARIO de la ra√≠z. Sigue el fundido negro/blanco.
El fondo original se conserva para HD apagado o recurso ausente.

Windows32 y64 compilados, ABI/boot y check_sources pasan. Capturas privadas
`pruebas/jiminy23-header` repiten la ruta amplia Auto4 de seis vistas por
arquitectura. Contin√∫an pendientes PAGE, otras cabeceras y borde del libro.
Este bloque es parcial y est√° incluido en2.3-pre1 junto al contador de p√°gina.

Tercer bloque: ayuda inferior del diario con las acciones reales de abrir,
volver, cambiar entrada y cerrar. Conserva los 240√ó160 p√≠xeles del juego y
reserva catorce filas para la ayuda, con una transformaci√≥n com√∫n para todas
las capas. Incluye seis idiomas y etiquetas de los controles configurados.
Si falta la fuente o su textura no est√° lista, usa la fuente nativa sin crear
otra textura. Original, CRT y HUD HD apagado conservan la presentaci√≥n previa.

El fundido usa la paleta capturada del fotograma presentado: despu√©s de
VBlank la paleta viva puede pertenecer ya al siguiente fotograma. Una captura
durante la transici√≥n reproduc√≠a ayuda negra sobre el libro visible; el
control corregido limita las diferencias al footer. La prueba CPU ejercita
las funciones reales con mutaci√≥n de la paleta siguiente, negro, blanco y
forced blank. La geometr√≠a cubre salidas peque√±as, amplias, altas y no enteras;
la disponibilidad de glifos cubre ausencia, espacio y rechazo del bridge.

Evidencia privada: `pruebas/jiminy23-footer-snapshot32`, `snapshot64`,
`snapshotFractional64`, `snapshotFourthree64` y `snapshotOff32` (mismo prefijo).
Cinco recorridos reales terminaron con c√≥digo 0 y cinco capturas cada uno;
el control HD apagado coincide p√≠xel por p√≠xel con las cinco vistas anteriores.
No se modific√≥ la partida original. Esto no termina la p√°gina derecha, PAGE,
otras cabeceras, las extensiones ni el estiramiento de p√≠xeles del libro.
La pre2.3-pre1 publicada permanece inmutable y no contiene este tercer bloque.

Cuarto bloque: las extensiones de cabecera, bandas y pesta√±as conservan sus
colores y posici√≥n junto al fotograma presentado. Antes, s√≥lo la pesta√±a BG
se capturaba antes de VBlank; el resto pod√≠a leer la paleta/VRAM del siguiente
fotograma o descriptores de objetos ya liberados. La captura nueva usa un
buffer est√°tico menor de40 KiB, limitado por las coordenadas OAM originales,
sin asignaci√≥n por ventana y sin alterar la l√≥gica ni el framebuffer nativo.

`test_menu_presented_margins.py --baseline-ref d0180e699` falla tras mutar los
datos del siguiente fotograma; la versi√≥n nueva conserva cabecera m√≥vil,
bandas y pesta√±as, y borra la captura al presentar una imagen sin ellas.
Comprueba l√≠mites y todos los tama√±os/formas OAM a lo largo de512 posiciones.
Las26 capturas reales del diario (32 y64) terminan con c√≥digo0. En las diez
vistas comparables, s√≥lo cambia el rect√°ngulo de la cabecera izquierda que
antes quedaba negro durante la transici√≥n1721; el resto es id√©ntico.

Evidencia privada: `pruebas/jiminy23-margin32`, `jiminy23-margin64` y
`jiminy23-pages/MARGIN-PIXEL-SCOPE.json`. Una prueba adicional abre el mapa
mediante START/direcci√≥n/A desde una sala inicializada y termina con cuatro
capturas y c√≥digo0 en64. La cadena artificial que forzaba cinco modos sin
preparar el mapa falla tambi√©n en el binario anterior, en el mismo acceso
nulo de AllmapSetBounds; se conserva como control inv√°lido, no como una
regresi√≥n nueva ni como validaci√≥n de los men√∫s posteriores al fallo.
Ver `jiminy23-pages/MARGIN-HARNESS-CRASH.json`. Siguen pendientes el libro,
PAGE y las dem√°s cabeceras; este bloque tampoco cierra P2.
Las cuatro vistas del mapa abierto con controles normales coinciden con el
binario anterior. Otra prueba64 prepara una sala v√°lida y abre la entrada
de la tienda Moguri, con cuatro capturas y salida0; s√≥lo cubre su presentaci√≥n,
no el audio ni todos sus submen√∫s. Windows32/64 y las bibliotecas Android
US/JP est√°n compilados; los contratos ABI/boot64 y check_sources pasan.


Quinto bloque: papel y anillas de las tres p·ginas de detalle (historia,
naipes y personajes). Se reconstruyen las hojas con borde derecho redondeado
y reglas en sus posiciones originales; las quince anillas proceden del
recurso equivalente del remaster. La receta conserva el RGB555 del PPU,
incluida su expansiÛn de bits; no cambia los colores de recetas anteriores.
Este recurso se limita a EU/US. JapÛn conserva el fondo nativo hasta verificar
su equivalencia visual; compilar su biblioteca no certifica esa equivalencia.

La sustituciÛn sÛlo ocupa pÌxeles cuyo ganador real es BG1 opaco. Una m·scara
capturada junto al fotograma mantiene por encima textos, retratos, naipes,
otras capas y combinaciones alfa. TambiÈn conserva cambios de la interfaz
hechos despuÈs de componer la imagen. RaÌz/listas y mapas de detalle todavÌa
no presentados no activan este recurso. Las tres variantes de mapa se reconocen
por separado: pueden usar Ìndices de tiles distintos aunque dibujen lo mismo.

El primer plano conserva el lienzo completo240◊160, con cabecera transparente;
recortar a144 filas desplazaba filas del sprite al aplicar escalas no enteras.
Una comparaciÛn real32/64 del sprite de Sora pasa de diferencias en427 pÌxeles
(32 software) a identidad exacta en el recorte de control. La cabecera nativa
se recorta en destino conservando su transformaciÛn completa. El filtro del
primer plano sigue la opciÛn gr·fica, incluso al cambiarla con textura viva.
La textura es ˙nica y reutilizable; se libera con los dem·s recursos HD.

El lookup da prioridad a la definiciÛn exacta frente a una imagen que sÛlo
comparte tiles. Si existe una definiciÛn exacta pero su recurso no est· listo,
conserva el nativo, sin mostrar el fondo del libro como cabecera. La prueba
CPU falla con el lookup anterior y pasa con el nuevo. Las pruebas reales de
composiciÛn/dibujo inyectan errores de asignaciÛn, mezcla, filtro, subida,
bridge, geometrÌa y cabecera: todos recuperan la imagen nativa completa.

El importador C++ real actualiza una copia privada del import anterior de6
recetas pendientes a0:889 archivos,883 previos idÈnticos en bytes, cinco
cabeceras reescritas con pÌxeles idÈnticos. El libro nuevo coincide exactamente
con la implementaciÛn Python. No se reimporta vÌdeo/m˙sica ni se modifica
la instalaciÛn original. Evidencia privada: jiminy23-book-colors-native y
jiminy23-book-colors; sÛlo recetas/cÛdigo entran en Git.

Controles privados: jiminy23-pages/BOOK-NATIVE-FALLBACK-CONTROLS.json contiene
15 comparaciones idÈnticas con HD apagado o recurso ausente. Los recorridos
jiminy23-book-transform-verified (32/64), transform-fractional y
transform-fourthree terminan con cÛdigo0 y cinco vistas cada uno. RaÌz/lista
y la cabecera mantienen sus pÌxeles; cambios limitados al cuerpo del libro.
BOOK-TRANSFORM-PIXEL-CONTROLS.json conserva diez comparaciones y dos recortes
de Sora idÈnticos. La versiÛn final que respeta el filtro se vuelve a capturar
en jiminy23-book-reviewed. Estas pruebas usan audio dummy y no validan audio.

Windows32/64 y bibliotecas Android US/JP compiladas; contratos ABI/boot64,
composiciÛn, lookup, geometrÌa, fundido y check_sources pasan. No se afirma
validaciÛn fÌsica de Android/consolas. P2 sigue parcial: PAGE, cabeceras de
historia/personajes, mini-header, raÌz/listas y verificaciÛn Riku/JP pendientes.
La pre2.3-pre1 publicada no contiene este bloque ni se sobrescribe.
