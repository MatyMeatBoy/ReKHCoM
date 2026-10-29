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


Quinto bloque: papel y anillas de las tres páginas de detalle (historia,
naipes y personajes). Se reconstruyen las hojas con borde derecho redondeado
y reglas en sus posiciones originales; las quince anillas proceden del
recurso equivalente del remaster. La receta conserva el RGB555 del PPU,
incluida su expansión de bits; no cambia los colores de recetas anteriores.
Este recurso se limita a EU/US. Japón conserva el fondo nativo hasta verificar
su equivalencia visual; compilar su biblioteca no certifica esa equivalencia.

La sustitución sólo ocupa píxeles cuyo ganador real es BG1 opaco. Una máscara
capturada junto al fotograma mantiene por encima textos, retratos, naipes,
otras capas y combinaciones alfa. También conserva cambios de la interfaz
hechos después de componer la imagen. Raíz/listas y mapas de detalle todavía
no presentados no activan este recurso. Las tres variantes de mapa se reconocen
por separado: pueden usar índices de tiles distintos aunque dibujen lo mismo.

El primer plano conserva el lienzo completo240×160, con cabecera transparente;
recortar a144 filas desplazaba filas del sprite al aplicar escalas no enteras.
Una comparación real32/64 del sprite de Sora pasa de diferencias en427 píxeles
(32 software) a identidad exacta en el recorte de control. La cabecera nativa
se recorta en destino conservando su transformación completa. El filtro del
primer plano sigue la opción gráfica, incluso al cambiarla con textura viva.
La textura es única y reutilizable; se libera con los demás recursos HD.

El lookup da prioridad a la definición exacta frente a una imagen que sólo
comparte tiles. Si existe una definición exacta pero su recurso no está listo,
conserva el nativo, sin mostrar el fondo del libro como cabecera. La prueba
CPU falla con el lookup anterior y pasa con el nuevo. Las pruebas reales de
composición/dibujo inyectan errores de asignación, mezcla, filtro, subida,
bridge, geometría y cabecera: todos recuperan la imagen nativa completa.

El importador C++ real actualiza una copia privada del import anterior de6
recetas pendientes a0:889 archivos,883 previos idénticos en bytes, cinco
cabeceras reescritas con píxeles idénticos. El libro nuevo coincide exactamente
con la implementación Python. No se reimporta vídeo/música ni se modifica
la instalación original. Evidencia privada: jiminy23-book-colors-native y
jiminy23-book-colors; sólo recetas/código entran en Git.

Controles privados: jiminy23-pages/BOOK-NATIVE-FALLBACK-CONTROLS.json contiene
15 comparaciones idénticas con HD apagado o recurso ausente. Los recorridos
jiminy23-book-transform-verified (32/64), transform-fractional y
transform-fourthree terminan con código0 y cinco vistas cada uno. Raíz/lista
y la cabecera mantienen sus píxeles; cambios limitados al cuerpo del libro.
BOOK-TRANSFORM-PIXEL-CONTROLS.json conserva diez comparaciones y dos recortes
de Sora idénticos. La versión final que respeta el filtro se vuelve a capturar
en jiminy23-book-reviewed. Estas pruebas usan audio dummy y no validan audio.

Windows32/64 y bibliotecas Android US/JP compiladas; contratos ABI/boot64,
composición, lookup, geometría, fundido y check_sources pasan. No se afirma
validación física de Android/consolas. P2 sigue parcial: PAGE, cabeceras de
historia/personajes, mini-header, raíz/listas y verificación Riku/JP pendientes.
La pre2.3-pre1 publicada no contiene este bloque ni se sobrescribe.

Sexto bloque: completar las cabeceras HISTORIA/PERSONAJES y sus equivalentes
en inglés, francés, italiano y alemán. Los diez rótulos se leen de las
variantes del cartucho (incluido GESCHICHTE) y se reconstruyen con las mismas
letras remasterizadas que la cabecera de naipes. Las cinco recetas de naipes
anteriores se mantienen idénticas. El registro elige el mapa real de cada
una de las tres páginas, respetando idioma y pasos del fundido.

`test_jiminy_detail_headings.py` ejecuta ese fragmento real: falla con el
registro anterior limitado a naipes y pasa2880 combinaciones EU/US de tres
páginas, cinco idiomas y32 pasos negros/blancos/aditivos; también mantiene
el registro apagado en JP y en la compilación sin PLATFORM_PC. Generación y revisión
de los diez PNG; el importador C++ real añade diez recursos, pasa de10 recetas
pendientes a0 y mantiene889 salidas previas idénticas en bytes. Siete imágenes
coinciden exactamente con Python; las otras tres difieren en un solo píxel
RGB y un nivel por canal, con alfa idéntica (tolerancia de muestreo). La prueba
inicial exigía igualdad estricta y se conserva su fallo en la evidencia;
no se ha cambiado el importador general por esa tolerancia.

Diez capturas US32/64 terminan con código0: sólo cambian las dos cabeceras
nuevas, dentro de las primeras16 filas; raíz/lista/naipes y el cuerpo del libro
son idénticos al bloque500. Evidencia privada: jiminy23-headings-runtime,
jiminy23-heading-import, jiminy23-heading-native y
jiminy23-pages/HEADING-PIXEL-SCOPE.json. La ruta europea en español con copia de guardado EU termina con cinco
capturas, desbloqueo en RAM confirmado y código0 (headings-spanish-eusave); no se confunde la revisión de diez PNG con cinco recorridos
reales. Audio dummy, sin certificación de sonido ni hardware de consola.
Las notas de los bloques previos se normalizan a UTF8 sin cambiar sus reglas.

P2 continúa: PAGE, mini-header, raíz/listas y equivalencia visual Riku/JP.
La pre2.3-pre1 anterior se mantiene inmutable; estos cambios requieren otra
compilación futura y no acreditan la entrega estable2.3.


Séptimo bloque: pestaña PAGE/PÁGINA/PAGINA/SEITE. Su caja es151,0..240,8;
los botones L/R empiezan en5+3=8 y los dígitos quedan debajo. La receta comprueba
esos límites con los frames originales. Reconstruye sólo esa franja con borde
redondeado, colores RGB555 del juego y letras remasterizadas. El fondo BG1
que despeja las letras y esquinas nativas coincide en las tres páginas; las
dos variantes del overlay PAGE también coinciden. PÁGINA conserva su acento
y la fila inferior original del texto español. No cambia valores ni controles.

Se registra como una segunda imagen estática, con una llave exacta distinta
del título de página. El contrato CPU real falla sin ese registro y pasa las
combinaciones EU/US de las tres páginas, cinco idiomas y todos los pasos de
fundido; JP y la compilación de cartucho conservan su ruta nativa. Lookup
específico y textura ausente mantienen el fallback nativo. El importador C++
añade cinco archivos, de5 recetas pendientes a0 (904 imágenes);899 salidas
previas se mantienen idénticas en bytes y los cinco PNG coinciden exactamente
con Python. La corrección final sólo cambia el recurso español:903 previos
idénticos y cinco comparaciones de píxeles exactas.

Diez capturas US32/64 sólo cambian el rótulo dentro de las primeras8 filas.
Los recortes de contadores y L/R son idénticos, igual que raíz/lista. Cinco
vistas adicionales64 con PAGE ausente coinciden con main501. Diez vistas64
en ventana fraccional y4:3 terminan con código0; seis recortes de contadores
y L/R coinciden con el control anterior. España EU se prueba con una copia
del respaldo europeo y desbloqueo sólo en RAM. Las pruebas usan audio dummy;
no acreditan audio ni consolas físicas. Evidencia privada: jiminy23-page-*,
PAGE-PIXEL-SCOPE.json, PAGE-MISSING-PIXEL-CONTROL.json y
PAGE-FRACTIONAL-CONTROLS.json en jiminy23-pages.

Windows EU32/US32/64 y bibliotecas Android US/JP compiladas; ABI/boot64,
registro, lookup y check_sources pasan. La última corrección de la tilde sólo
cambia la receta externa española y el importador, sin cambiar el código del
juego ni las otras cuatro recetas. Continúan el nombre en amarillo, las
vistas de cubierta/listas y la cobertura Riku/JP, además del resto del plan2.3.
El bloque no modifica los paquetes publicados ni cierra P2.


Octavo bloque: el nombre amarillo no determina sus márgenes por longitud.
La regla anterior de bandas comunes copiaba una franja al margen derecho
con nombres largos y dejaba incompleto el margen izquierdo con nombres
cortos. El compositor conserva24 colores por lado, sólo cuando BG0 opaco
gana realmente en el borde del fotograma presentado. Son192 bytes estáticos;
no hay asignación, textura nueva ni modificación del framebuffer original.

La presentación extiende a la izquierda el recuadro real. A la derecha usa
su BG0 legítimo, si lo hay, o el fondo de ese margen. El PAGE derecho no se
confunde con un nombre de la izquierda. El color ya incluye el fundido del
fotograma; no se consulta la paleta del próximo VBlank. La corrección se
limita al diario amplio con Cartas HD, y mantiene la ruta original apagada.

`test_jiminy_name_margins.py` ejecuta el renderer y PixelEdge reales más la
captura portable: propietario BG0, mezclas, posiciones fuera de rango,
negro opaco, metadatos ausentes y márgenes de ancho0. El caso de PAGE sólo
a la derecha no genera una extensión del nombre. El contrato anterior no
tenía ese renderer y la reproducción visual anterior conserva la franja
incorrecta. El contrato de composición y fallback del libro sigue pasando.

Treinta pares privados de capturas comparan nombres cortos y largos en
US32/64, EU español, ventana fraccional,4:3 y HD completamente apagado.
Sólo cambian los márgenes a la altura del nombre: libro, retratos, texto,
PAGE y contadores mantienen sus píxeles. La cobertura de rectángulos del
renderer software termina en columna1363; la de centros de píxel GPU en1364.
La primera aserción de una cobertura común incluyó una columna de la franja
errónea anterior en software; se conserva ese diagnóstico. Las columnas
reales del libro son idénticas. El primer control histórico apagado dejaba
texto HD activado y se excluye; un control nuevo con el mismo preset y todos
los interruptores HD apagados sí coincide exactamente.

Evidencia: NAME-MARGIN-PIXEL-SCOPE.json, NAME-MARGIN-MATRIX-CONTROLS.json y
NAME-MARGIN-OFF-CONTROLS.json en pruebas privadas jiminy23-pages; recorridos
jiminy23-name-*. Todas las partidas originales se conservan. Windows
EU32/US32/64 y bibliotecas Android US/JP compiladas, ABI/boot64, fuentes y
contratos CPU pasan. Audio dummy, sin afirmar mejoras de sonido ni prueba
física de Android/consolas. Continúan cubierta/listas y cobertura Riku/JP
antes de cerrar P2 y pasar al audio; los releases anteriores no se modifican.

Noveno bloque: las cabeceras de Riku tenían cinco frames propios sin receta
HD. Se añaden D REPORT, RAPPORT D, INFORME D, DIARIO D y D-BERICHT con los
textos, colores y cajas nativos. La F española procede de la letra real de
STUFENANSTIEG en el atlas remasterizado. No se cambia la cabecera de Sora ni
se renombra ningún recurso anterior; las recetas se añaden por separado del
grupo cuya regla de nombres depende del número de frames.

El test de recetas falla antes por ausencia de la función de Riku y pasa con
los cinco bindings, sus cajas originales y las guardas EU/US. El importador
C++ detecta cinco pendientes y llega a cero: los904 outputs anteriores
(902PNG y2layouts LAD) conservan sus bytes. Las cinco imágenes nuevas
coinciden con la referencia: dos exactas y tres con como máximo un nivel RGB
de diferencia, con alfa idéntica. No se distribuyen las imágenes extraídas.

Capturas reales de raíz/lista en los cinco idiomas EU32 y en US64 cambian
únicamente la cabecera. El estado de Riku se prepara con la ruta de nueva
partida del menú de pruebas en RAM, sin editar guardados. Las páginas de
detalle y HD apagado tienen controles separados; no confundir una fuente
original conservada por falta del recurso con el nuevo reemplazo aceptado.

Los controles apagados coinciden en32/64, y el detalle de historia de Riku
también es idéntico antes/después. Un primer control64 simultáneo agotó el
plazo del observador; no se toma como prueba aprobada ni como causa de un
crash. Sus capturas/log se conservan. Los controles anteriores/nuevos64
secuenciales alcanzan las cinco capturas programadas y terminan con código0,
sin modificar el binario entre intentos. No se asigna causa al driver.

Evidencia privada: pruebas/jiminy23-riku y sus controles runtime/detail/off.
Windows EU32/US32/64 y Android US/JP compilados; ABI/boot64 y fuentes pasan.
Esto no certifica hardware Android/consolas ni audio. Continúan cubierta/listas,
la cabecera alemana de Sora aún sin receta, y cobertura JP antes de cerrar P2.

Décimo bloque: el frame8 alemán de Sora dice REISECHRONIK en el original.
Faltaba su binding HD, mientras las otras nueve cabeceras Sora/Riku ya estaban
cubiertas. Se añade una sola receta EU con la caja original104×16 y letras
remasterizadas. La prueba de las diez cabeceras falla antes en ese frame y
pasa después; las recetas y nombres anteriores se conservan.

La actualización C++ real pasa de1 pendiente a0. Los909 outputs previos
mantienen sus bytes y la imagen nueva es idéntica a la referencia Python.
El primer ensayo de importación agotó120s mientras seguía escribiendo;
se conservó su registro. Sin compilaciones simultáneas, el ensayo completo
terminó en59.39s, con progreso registrado y validación final aprobada.
Eso no acredita una mejora de rendimiento ni un bloqueo del producto.

Capturas EU32/software y EU64/Direct3D11 de portada, Naipepedia y HD apagado
confirman cambios sólo en la cabecera. El primer perfil pulsaba Historia,
que está bloqueada en esa nueva partida, y sólo verifica portada; el perfil
corregido selecciona Naipepedia y comprueba que la página realmente se abre.
No se cuenta esa selección fallida como evidencia de lista.

Evidencia privada: pruebas/jiminy23-german, NATIVE-UPGRADE.json, runtime y
runtime-list. EU32/64, JP32/64 y Android US/JP compilados; ABI/boot64 y
check_sources pasan. La ejecución japonesa se verifica por separado; las
recetas europeas no afirman convertir su arte ni sus textos a HD.
