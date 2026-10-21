# Preliminar V1 — avance por pieza

Solicitud: todas las escenas que necesitan extensión y todos los jefes
pendientes. Código real listo para integrar, capturas x3, copia jugable y
commits de Codex; Claude integra. La revisión del usuario es conjunta.

## Método de cierre

Terminar **una pieza por vez**: estudiar sus capas originales, aislar donantes
completos, implementar, comparar una misma instantánea, corregir visualmente,
comprobar cuatro ventanas y vista Original, documentar y hacer commit.
No pasar a la siguiente por el solo hecho de que compile. Una propuesta de
Preliminar V1 no equivale a aprobación del usuario.

El selector `KHCOM_PRELIMINAR_V1=1` solo activa perfiles promovidos en `READY`
de `port/tools/preliminar_v1_recipes.py`. Los demás parámetros son estudios
pendientes, desactivados. Los estudios iniciales de jefes quedan en archivos
privados de `build/preliminar-v1`, fuera del renderizador entregado.
El inventario acepta `--region eu|us|jp`; al generar la tabla C se combinan
los tres archivos `scenes.json` para que cada perfil cubra los hashes de las
tres ediciones del juego.

## Inventario

59 fondos distintos en las tablas de eventos: 58 de tiles de texto y uno
afin. Se identifican mapas por sus datos y dimensiones, incluido EU que los
descomprime en RAM. Las escenas compartidas usan una sola receta.

- Aprobadas anteriormente: 004, 057, 061, 074, 107/108 y 109.
- Cerradas para comentarios: 007 (también evento 009), 044 (mapa compartido con 186),
  045, 053 (también evento 177), 054 (también evento 178) y 055 (eventos 056,
  179, 181 y 183), 081 (eventos 082–085), 087 (evento 088), 095, 097, 098,
 099 y 102, 110 (también evento 111), 112 (también evento 114), 115, 116.
- Pendientes de ciclo completo:
  117, 118, 120, 121, 122, 124, 129, 131 y 132.
- Mantener límites dibujados: habitaciones del castillo, 006 (sala delimitada),
  075/086 (tesoro), 076 (cámara circular), 089 (recinto del cementerio),
  091 (puzzle), 094 (casa), 101 (plataforma de hielo), 103 (recinto marino),
  130 (biblioteca), paneles 073/149/173/189 y variantes del castillo. Las
  islas de 054/055 se conservan; se estudia solo el agua/cielo de alrededor.
- Jefes pendientes: Darkside, Oogie Boogie, Jafar, Maléfica dragón, Úrsula,
  Trickmaster (solo suelo inferior), Marluxia final.
- Pendiente en esta tanda: 9 fondos y 7 escenarios de jefe; 16 piezas en total.
- Conservar Guard Armor aprobado, Hook y Monstro según indicación previa.

## Registro 007

Patio de Traverse Town, escena de Cid. Suelo de rombos, muro diagonal a la
izquierda y fachada horizontal a la derecha. No duplicar la puerta, torre,
castillo, personajes ni cajas.

- BG3 izquierdo: período de 12 tiles por la diagonal 2:1; contiene dos
  módulos completos del muro y casa con el período del pavimento.
- BG2 izquierdo: tira limpia de 6 tiles, sin las cajas del mapa original.
- BG2 derecho: solo la franja de luminarias de 7 tiles. El resto de la capa
  no se repite, por lo que las cajas permanecen únicas.
- El parapeto del fondo mantiene su horizontal y su color; no se arrastra
  por la diagonal del muro delantero.
- Aplicación solo fuera de los 240×160 GBA. Las paletas se leen activas.

Las pruebas y capturas privadas están en `build/preliminar-v1/`.
El estado de cierre se actualizará con los resultados, sin presentar los
estudios pendientes como resultados listos para integrar.

Verificación de cierre 007:

- EU/US/JP: 50 pares de instantáneas antes/después comprobados. Vista amplia
  en los cuatro tamaños y tres estados (100/300/1000) en las tres regiones;
  Original en los cuatro tamaños EU y una muestra US/JP. Cero cambios en
  los 240×160; Original idéntica. Las ampliaciones sí cambian donde corresponde.
- Controles EU: 004, 006, 014, 057, 061, 074, 107, 109 y 130 sin cambios en
  toda la imagen al activar la tanda. Una partida/animación completa no se
  deduce de estas instantáneas.
- Refinamientos finales: color de fondo tras el muro igual al original;
  reconstruir los 32 px que faltaban de la primera casa con una crujía completa,
  incluyendo el hueco anterior. No arrastrar una casa cortada a cada repetición.
- Capturas finales EU: `build/preliminar-v1/final-007/escena-007`.
- Copia jugable privada: `../ReKHCoM-preliminarV1`, lanzador
  `Probar Preliminar V1.bat`. Contiene solo piezas cerradas en su menú.
- Estado: preliminar implementada para comentarios, no aprobada por el usuario.

## Registro 044

Plaza de Ciudad del Paso al atardecer. El reloj y la silueta central son
únicos. Se conservan ventanas especiales, farolas y jardineras originales.
El perfil se asocia a los datos compartidos por 044/186; las capturas recorren
044, no toda la secuencia de 186.

- Cielo desde una tira completa sin edificios ni el relleno marrón lateral.
- Tejado rojo, fachada amarilla diagonal y fachada derecha horizontal con
  sus continuidades independientes. El edificio central no se replica.
- Borde de ladrillo izquierdo desde x=120..127, un hueco entre jardineras:
  muestrear en píxeles conserva los desplazamientos alternos de 4 px. Copiar
  tiles completos introducía trozos de maceta o escalones en la diagonal.
- El suelo sigue la franja de sol por la diagonal opuesta a la fachada;
  los donantes de sombra proceden del suelo real, no del relleno del mapa.
- Paleta BG activa y cambios solo fuera del rectángulo GBA.

Verificación: 16 casos / 27 pares pasan, sin diferencias en los 240×160.
EU: cuatro ventanas x3 y estados 100/300/1000. US/JP: una muestra amplia x3
3440×1440 en el estado 300; vista Original de las tres regiones. No equivale
a probar todas las fases o todas las cámaras de las escenas compartidas.
Los diez controles (004/006/007/014/057/061/074/107/109/130) permanecen
idénticos en toda la imagen al activar solo la receta 044.

Evidencia privada: `build/preliminar-v1/final-044*`. Se comparó la misma
instantánea y el mismo ejecutable en cada par. `KHCOM_PRELIMINAR_SKIP=44`
permite apagar solo esta receta manteniendo activadas las anteriores.

## Registro 045

Entrada al bosque del castillo. El castillo, verja, sendero central y grupo
de personajes se mantienen únicos. La expansión usa la paleta y los gráficos
de tiles activos del mapa original; no añade sprites ni imágenes rasterizadas.

- BG3 extiende el cielo y la línea distante de abetos y verja con bandas
  separadas. El sendero continúa por su diagonal y la hierba rellena el resto.
- BG2 continúa la verja horizontal. Los abetos de primer plano se forman a
  partir de cinco apariciones originales parcialmente solapadas: se combinan
  los índices de paleta más consistentes y se conserva el componente conectado
  a la punta, para no repetir copas cortadas. Se dibujan solo fuera del mapa.
- La caché del abeto se deriva de los bytes activos de los tiles y se invalida
  cuando estos cambian; el color se obtiene de la paleta en cada dibujo.

Verificación: 12 pares por región (EU, US y JP), cuatro ventanas y estados
100/300/1000; cero diferencias en los 240×160 nativos. El modo Original da
imagen idéntica en los cuatro tamaños EU. Controles completos de 007 y 044
siguen pasando en EU. Esto cubre el evento 045 y esos estados, no toda la
secuencia compartida 046/187/188 ni cada cámara o transición.

Evidencia privada: `build/preliminar-v1/verify-045*`,
`build/preliminar-v1/original-045` y `build/preliminar-v1/regression-007-044`.
La copia jugable contiene ahora 007, 044, 045 y 053. Estado: preliminar para
comentarios; no aprobada por el usuario. Las escenas restantes y los siete
jefes siguen pendientes.

## Registro 053

Tramo de playa de Isla del Destino, compartido también con el evento 177 de
Riku. Se conserva la colocación de las palmeras, arbustos y personajes.

- BG3 continúa muro, vegetación, arena y terreno siguiendo la diagonal
  original. Usa bandas laterales limpias del muro y texturas de arena y suelo
  fuera de la zona ocupada; las palmeras no se repiten en la extensión.
- BG2 sigue la orilla y el patrón de olas con la misma fase diagonal; el agua
  profunda se rellena con el azul del propio mapa.
- La pendiente al trazar x mantiene la línea del muro y la playa paralelas a
  la costa en ambos extremos.

Verificación: 12 pares en EU, US y JP, cuatro ventanas y estados 100/300/1000;
cero diferencias nativas. El evento 177 pasa los mismos 12 pares en EU con
sus personajes y cámara propios. El modo Original resulta idéntico en los
cuatro tamaños EU. La prueba cubre esos inicios y estados, no toda la secuencia
ni cada cámara de ambos eventos.

Evidencia: `build/preliminar-v1/verify-053*`,
`build/preliminar-v1/original-053` y capturas del evento 177 bajo
`build/preliminar-v1/verify-053-event177`. Estado: listo para comentarios;
no aprobado por el usuario.

## Registro 054

Playa de Isla del Destino, compartida entre el inicio 054 y el evento 178 de
Riku. La isla, su costa, las palmeras, la vegetación y la pasarela se conservan
como elementos únicos; la receta extiende solo el cielo y el océano.

- BG2 queda limitado a los 64×64 tiles originales, manteniendo intacta la
  silueta completa de la isla y de la pasarela.
- BG3 conserva su contenido original dentro del mapa. En los bordes, el cielo
  y las nubes usan una muestra de cielo despejado; el mar toma un tramo limpio
  de 16 tiles de ancho para espaciar las olas y evitar repetir la isla.
- Las palmeras y el acantilado no aparecen en los donantes de mar; el horizonte
  continúa en una línea horizontal sin escalones.

Verificación: ambos inicios, en EU/US/JP, cuatro ventanas y estados 100/300/1000;
12 pares por evento y región, con extensión activa y cero diferencias en los
240×160 nativos. Vista Original idéntica en los cuatro tamaños EU para ambos
eventos. No cubre todos los fotogramas de las secuencias.

Evidencia: `build/preliminar-v1/verify-054-eu`, `verify-054-us`,
`verify-054-jp` y `original-054-eu`. Captura final:
`build/preliminar-v1/verify-054-eu/escena-054/3440x1440-300-despues.png`.
Estado: preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 055

Escena del colapso de Isla del Destino, compartida por 055, 056, 179, 181 y
183. Se conserva la atmósfera oscura de la escena: BG3 prolonga lateralmente
la franja de nubes moradas y cielo negro; BG2 mantiene la palmera y los restos
únicos en la isla. No se copia ningún fragmento del acantilado ni de los restos.

- La banda lateral usa solo las primeras ocho filas de tiles de BG3, por encima
  de la isla. El resto del espacio exterior conserva el cielo oscuro original.
- BG2 queda limitado al mapa original para evitar duplicar palmera y escombros.
- Se mantiene el negro que rodea la isla dentro del mapa; rellenarlo de turquesa
  crea un corte rectangular frente al cielo oscuro dibujado por la escena.

Verificación: los cinco inicios compartidos pasan 12 pares EU cada uno, cuatro
ventanas y estados 100/300/1000; cero diferencias en los 240×160 nativos. El
inicio 055 también pasa 12 pares en US y JP y el control Original EU resulta
idéntico. Los cinco inicios pasan además la muestra rápida 3440×1440 en las tres
regiones; esto no recorre todas las fases de sus secuencias.

Evidencia: `build/preliminar-v1/verify-055-eu`, `verify-055-us`,
`verify-055-jp`, `verify-055-shared-eu` y `original-055-eu`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 081

Pasillo orgánico de Monstro E3, compartido entre los inicios 081–085 (combate,
victoria, dos fallos y reintento). BG3 prolonga el piso y BG1 el borde
orgánico con bandas diagonales de ocho tiles; BG2 continúa el tejido y las
manchas del entorno. Los personajes, enemigos y diálogos permanecen ligados a
cada inicio de evento. La repetición de textura en la extensión queda como
punto de comentario visual.

Verificación: los cinco inicios pasan las 12 comparaciones EU (cuatro ventanas,
estados 100/300/1000) con cero diferencias en el área nativa 240×160. Los cinco
pasan también la muestra rápida en US; el inicio 081 pasa la muestra rápida en
JP. Vista Original: los cinco inicios pasan muestra rápida EU sin diferencia
nativa.

Evidencia EU: `build/preliminar-v1/ready/escena-081` y
`evento-082-escena-081` a `evento-085-escena-081`. Captura:
`build/preliminar-v1/ready/escena-081/3440x1440-300-despues.png`. Estado: preliminar
implementada para comentarios; no aprobada por el usuario.


## Registro 087

Sala oscura de Halloween Town E0, compartida por los eventos 087 y 088. La
sala original mantiene sus dos estatuas, el muro superior y el muro diagonal
del frente. BG2 queda dentro del recinto. BG3 extiende solo el piso con un
tramo limpio de la textura interior; una fase determinista alterna sus ocho
filas cada bloque horizontal para que no aparezca siempre el mismo mosaico.

Verificación: ambos eventos pasan 12 comparaciones EU (cuatro ventanas, estados
100/300/1000), con cero diferencias nativas. Ambos pasan la muestra rápida en
US y JP. Vista Original EU: 087 pasa las 12 comparaciones y 088 la muestra
rápida, sin diferencias nativas.

Evidencia EU: `build/preliminar-v1/ready/escena-087` y
`evento-088-escena-087`. Captura: `build/preliminar-v1/ready/escena-087/
3440x1440-300-despues.png`. Estado: preliminar implementada para comentarios;
no aprobada por el usuario.

## Registro 090

Cementerio de Halloween Town. BG3 reúne cielo, luna, muro, verja y piso en la
misma capa, así que no se repiten sus bordes completos. La luna, el portón y
las lápidas permanecen únicos; BG1 queda limitado al mapa original. La
extensión de BG3 usa tres donantes limpios: cielo y colinas sin luna, un tramo
de muro sin portón ni lápidas que continúa por la diagonal, y textura de piso
sin tumbas. Entre las colinas y el muro se conserva el fondo nocturno oscuro.

Verificación: evento 090 pasa 12 comparaciones EU (cuatro ventanas, estados
100/300/1000), sin diferencias en los 240×160 nativos. Pasa muestra rápida en
US y JP; Original EU pasa las 12 comparaciones sin cambios. Esto cubre la
entrada y esos estados, no cada cámara o fase de la escena.

Evidencia EU: `build/preliminar-v1/ready/escena-090`; capturas regionales en
`build/preliminar-v1/regional-090-us` y `regional-090-jp`, Original EU en
`build/preliminar-v1/original/escena-090`. Captura:
`build/preliminar-v1/ready/escena-090/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 095

Patio del laberinto de Wonderland, con el trono, el camino rojo, la reina,
los naipes, el arco, los árboles y las escaleras como elementos únicos. BG3
continúa el cielo con nubes, conserva una franja corta de setos a la distancia
y extiende la pradera desde un donante limpio del margen inferior. No se copian
el trono, el pasillo ni los fragmentos del laberinto que contienen objetos.

Verificación: escena 095 pasa 12 comparaciones EU (cuatro ventanas, estados
100/300/1000), sin diferencias en los 240×160 nativos. Pasa muestra rápida en
US y JP; Original EU pasa las 12 comparaciones sin cambios. La revisión visual
del usuario queda pendiente.

Evidencia EU: `build/preliminar-v1/ready/escena-095`; capturas regionales en
`build/preliminar-v1/regional-095-us` y `regional-095-jp`, Original EU en
`build/preliminar-v1/original/escena-095`. Captura:
`build/preliminar-v1/ready/escena-095/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 097

Bosque de 100 Acre. BG3 es una textura continua de suelo; se extiende desde
el tramo ocupado de 40×40 tiles para cubrir los laterales. BG2 conserva los
árboles, flores y piedras originales sin repetirlos. Los personajes mantienen
su posición y el perfil solo afecta a la vista extendida.

Verificación: escena 097 pasa 12 comparaciones EU (cuatro ventanas, estados
100/300/1000), sin diferencias en los 240×160 nativos. Pasa muestra rápida en
US y JP; Original EU pasa las 12 comparaciones sin cambios. La revisión visual
del usuario queda pendiente.

Evidencia EU: `build/preliminar-v1/ready/escena-097`; capturas regionales en
`build/preliminar-v1/regional-097-us` y `regional-097-jp`, Original EU en
`build/preliminar-v1/original/escena-097`. Captura:
`build/preliminar-v1/ready/escena-097/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 098

Sala de la Reina de Corazones. La extensión mantiene la chimenea, la mesa,
la puerta, la reina y los naipes en su lugar original. BG1 (la mesa y sus
objetos) queda limitado al mapa original. BG3 extiende el papel tapiz y el
piso ajedrezado con muestras limpias de sus propias superficies y una
transición diagonal que sigue la forma del salón; no replica los elementos
únicos de la habitación.

Verificación: escena 098 pasa 12 capturas EU (cuatro ventanas, estados
100/300/1000), y vista Original pasa sus 12 capturas sin cambios. US y JP
pasan la muestra rápida. El recorte GBA se verifica en cada captura.

Evidencia EU: `build/preliminar-v1/ready/escena-098`; capturas regionales en
`build/preliminar-v1/regional-098-us` y `regional-098-jp`, Original EU en
`build/preliminar-v1/original/escena-098`. Captura:
`build/preliminar-v1/ready/escena-098/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 099

Sala de la Reina de Corazones, batalla del evento 099. BG3 mezcla la pared,
la chimenea, el jarrón y el piso en un solo mapa. Los objetos y adornos
centrales permanecen únicos. A los lados se prolonga un solo módulo ornamental
y luego un panel rosa limpio. El piso ajedrezado usa dos colores opacos de la
paleta original, con celdas que crecen hacia el primer plano; así no se copian
la chimenea ni el punto de fuga central en los márgenes.

Verificación: escena 099 pasa 12 comparaciones EU (cuatro ventanas, estados
100/300/1000), y vista Original pasa las 12 sin cambios. US y JP pasan la
muestra rápida. El recorte GBA se mantiene idéntico en todas las capturas.

Evidencia EU: `build/preliminar-v1/ready/escena-099`; capturas regionales en
`build/preliminar-v1/regional-099-us` y `regional-099-jp`, Original EU en
`build/preliminar-v1/original/escena-099`. Captura:
`build/preliminar-v1/ready/escena-099/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 102

Sala submarina de Atlantica. BG3 conserva íntegra la sala y su corredor
diagonal dentro del encuadre GBA; en los márgenes, el renderizador sustituye
la silueta recortada por una extensión de suelo con textura de una muestra
continua y agua azul. BG2 mantiene el trono, las columnas y los corales como
elementos únicos, sin repetir módulos arquitectónicos. El recorte original
240×160 queda intacto.

Verificación: escena 102 pasa las 12 comparaciones EU (cuatro ventanas,
estados 100/300/1000), con recorte nativo idéntico; vista Original pasa sus
12 comparaciones. EU, US y JP pasan la muestra rápida. La revisión visual del
usuario queda pendiente.

Evidencia EU: `build/preliminar-v1/ready/escena-102`; capturas regionales en
`build/preliminar-v1/regional-102-us` y `regional-102-jp`, Original EU en
`build/preliminar-v1/original/escena-102`. Captura:
`build/preliminar-v1/ready/escena-102/escena-102/3440x1440-300-despues.png`.
Estado: preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 104

Cámara orgánica de Úrsula en Atlantica, compartida por las escenas 104 y 106.
BG3 mantiene completa la sala central y su boca única. En los márgenes se
prolonga el corredor con el período natural de 32 px en horizontal y 16 px en
vertical: así continúan las costillas diagonales sin reflejar ni duplicar la
boca. El muro usa su franja de estrías verticales; el respaldo inferior
continúa en púrpura sólido, sin espejar la fila vacía del mapa. BG2 conserva
los objetos y detalles propios. El encuadre GBA de 240×160 sigue idéntico.

Verificación: escenas 104 y 106 pasan 24 comparaciones EU (cuatro ventanas,
estados 100/300/1000); el recorte nativo permanece idéntico. La vista Original
pasa las 24 comparaciones. EU, US y JP pasan la muestra rápida en ambas
entradas.

Evidencia EU: `build/preliminar-v1/ready/escena-104` y
`build/preliminar-v1/ready/evento-106-escena-104`; capturas regionales en
`build/preliminar-v1/regional-104-us` y `regional-104-jp`; Original EU en
`build/preliminar-v1/original-104-eu`. Captura:
`build/preliminar-v1/ready/escena-104/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 110

Pasillo de Agrabah E2, compartido por los eventos 110 y 111. BG3 prolonga
los relieves de la pared con una banda amplia para reducir repeticiones y
continúa la diagonal de la pared con su misma pendiente; las tiras de borde
evitan volver a dibujar las columnas. El suelo abierto usa su propia muestra.
BG2 no se copia: jarrones, barriles, puestos y rocas de utilería siguen únicos.
El encuadre GBA de 240×160 permanece idéntico.

Verificación: escenas 110 y 111 pasan 24 comparaciones EU (cuatro ventanas,
estados 100/300/1000); el recorte nativo permanece idéntico. La vista Original
pasa las 24 comparaciones. EU, US y JP pasan la muestra rápida en ambas
entradas.

Evidencia EU: `build/preliminar-v1/ready/escena-110` y
`build/preliminar-v1/ready/evento-111-escena-110`; capturas regionales en
`build/preliminar-v1/regional-110-us` y `regional-110-jp`; Original EU en
`build/preliminar-v1/original-110-eu`. Captura:
`build/preliminar-v1/ready/escena-110/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 112

Salida de Agrabah, compartida por los eventos 112 y 114. BG3 prolonga la
superficie de arena con una muestra completa del suelo y conserva la pared
superior con un módulo limpio de BG2. Los andamios, la puerta, barriles,
personajes y demás utilería permanecen únicos: BG2 no se repite en los márgenes.
Los huecos sin capas visibles usan una muestra de pared arriba y suelo abajo;
esto elimina el bloque negro de las celdas transparentes sin cubrir el muro con
arena. El viewport GBA de 240×160 no cambia.

Verificación: eventos 112 y 114 pasan 24 comparaciones EU en vista amplia
(cuatro ventanas y estados 100/300/1000), con cero diferencias nativas. La vista
Original pasa las 24 comparaciones sin cambios. US y JP pasan una captura rápida
de cada evento a 3440×1440, estado 300. La evidencia regional queda registrada
en los informes de cada fixture; no cubre cada transición de los eventos.

Evidencia EU: `build/preliminar-v1/ready/escena-112` y
`build/preliminar-v1/ready/evento-114-escena-112`; capturas Original EU y
regionales US/JP bajo esos mismos directorios. Captura principal:
`build/preliminar-v1/ready/escena-112/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.

## Registro 115

Neverland E0, interior de la nave. BG3 continúa el entablado y la base de la
pared en sus diagonales originales. BG2 conserva el ritmo de ventanas y rejillas
de ventilación del corredor; los grupos grandes de barriles y los personajes
siguen siendo elementos únicos y no se copian. El perfil comparte hashes entre
EU, US y JP; el viewport GBA de 240×160 queda intacto.

Verificación: escena 115 pasa 12 comparaciones EU (cuatro ventanas, estados
100/300/1000); vista Original pasa sus 12 comparaciones sin cambios. US y JP
pasan una captura rápida a 3440×1440, estado 300. Esto verifica el inicio E0 y
los estados muestreados, no toda la secuencia de Neverland.

Evidencia EU: `build/preliminar-v1/ready/escena-115`; informes US, JP y Original
EU junto a la evidencia. Captura principal:
`build/preliminar-v1/ready/escena-115/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.\n

## Registro 116

Camarote del Capitán Garfio, evento 116. BG3 prolonga la arquitectura sin repetir
el mapa completo: pared de ladrillo sobre la diagonal y entablado bajo ella. BG2
conserva las ventanas, lámparas y objetos en sus posiciones originales; no se
repite porque contiene piezas únicas. El viewport GBA de 240×160 permanece intacto.

Verificación: escena 116 pasa 12 comparaciones EU (cuatro ventanas, estados
100/300/1000), Original pasa las mismas 12 sin cambios; US y JP pasan una muestra
3440×1440 en estado 300. Cero diferencias nativas en todas.

Evidencia EU: `build/preliminar-v1/ready/escena-116`; informes Original EU y
regionales US/JP junto a la evidencia. Captura principal:
`build/preliminar-v1/ready/escena-116/3440x1440-300-despues.png`. Estado:
preliminar implementada para comentarios; no aprobada por el usuario.
