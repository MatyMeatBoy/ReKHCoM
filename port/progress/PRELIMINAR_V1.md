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

## Inventario

59 fondos distintos en las tablas de eventos: 58 de tiles de texto y uno
afin. Se identifican mapas por sus datos y dimensiones, incluido EU que los
descomprime en RAM. Las escenas compartidas usan una sola receta.

- Aprobadas anteriormente: 004, 057, 061, 074, 107/108 y 109.
- Cerradas para comentarios: 007 (también evento 009), 044 (mapa compartido con 186),
  045, 053 (también evento 177).
- Pendientes de ciclo completo: 054, 055, 081, 087, 090,
  095, 097, 098, 099, 102, 104, 110, 112, 115, 116, 117, 118, 120, 121,
  122, 124, 129, 131 y 132.
- Mantener límites dibujados: habitaciones del castillo, 006 (sala delimitada),
  075/086 (tesoro), 076 (cámara circular), 089 (recinto del cementerio),
  091 (puzzle), 094 (casa), 101 (plataforma de hielo), 103 (recinto marino),
  130 (biblioteca), paneles 073/149/173/189 y variantes del castillo. Las
  islas de 054/055 se conservan; se estudia solo el agua/cielo de alrededor.
- Jefes pendientes: Darkside, Oogie Boogie, Jafar, Maléfica dragón, Úrsula,
  Trickmaster (solo suelo inferior), Marluxia final.
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
