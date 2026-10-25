# Lógica de los arreglos aprobados por el usuario (vista amplia)

Reglas que se repiten en todo lo que el usuario ha aprobado. Aplicarlas antes
de proponer algo nuevo; ante un caso nuevo, derivar la solución de ellas.

**Regla vigente de entrega (24/10):** aprender de todas las valoraciones aceptadas, pero activar arte nuevo para
jugadores solo desde la versión **con honores** (`build_version`) de la app. Cada entrega entra por PR y la
fusiona un integrador reservado; ver [COLA_COLABORATIVA.md](COLA_COLABORATIVA.md). Las menciones históricas a
"aprobado" o "Claude integra" no sustituyen esta regla. No elevar valoraciones ni heredar ★ a una versión hija.

**Mejora continua en cada PR (usuario 24/10):** todo PR de arte trae también su cambio a este archivo: el criterio
nuevo que dejó una valoración y la **corrección** de la regla cuando una pieza ya aprobada recibe comentarios
nuevos. Una aprobación con comentarios posteriores deja de ser modelo: su regla se reescribe con lo que pidió el
usuario y se revisan las escenas hermanas que la copiaron.

## Principios

Trickmaster, envío `20261025-052021-8546de`: «todo bien», sólo completar
el muro hacia arriba. Prolongar el período vertical del papel rosa, no la
franja beige del techo. Conservar suelo y objetos ya aceptados; la única
chimenea conserva su fuste de piedra donde cruza la prolongación del muro.

Escena 112, envío `20261024-054012-207689`: la franja superior que falta
en BG2 se rellena con la pared lisa de esa misma capa; una muestra superior
vacía no significa que termine el edificio. Completar la puerta única con
la prolongación vertical del color hasta el borde superior, como puerta muy
alta (solución temporal pedida explícitamente por el usuario el 25/10), y prolongar el
fuste ya existente sin colocar nuevas puertas ni sillas. La textura de BG2
tiene píxeles transparentes: continuar también su base rosa de BG3 para
que no deje ver el fondo amarillo. Los actores que
esperan fuera del plano usan la regla GBA ya integrada.
En la hoja derecha prolongada, usar el naranja del panel original señalado
por el usuario: no repetir la franja morada de separación por todo el ancho.

Escena 120, envío `20261024-053718-1ee415`: la silueta ocre de BG1
es parte del tejado delantero original, no un donante de suelo. Completar
sus pendientes laterales y su cuerpo inferior evita que quede una pieza
flotante con base horizontal cortada. El muro de BG3 continúa con sus
ladrillos limpios y un único pilar original; BG2 sólo prolonga el zócalo
por su diagonal, sin copiar lámparas ni adornos. Los actores siguen la
regla del rectángulo GBA (Pepito fuera de escena no se anticipa).

Integración 25/10: arenas de jefe aprobadas en Atlas (Maléfica `0cdb9c54`, Úrsula `fc418e25`, Marluxia `c333e5cd`)
quedan encendidas para todos (`PortArenasApproved`, `KHCOM_ARENAS_V2=0` las apaga). Las que siguen pendientes en la app
(Trickmaster, Oogie) quedan detrás de `KHCOM_ARENAS_V2=1`: una arena se enciende sola cuando su familia se aprueba.

Integración 25/10: escena 117, versión `ff9f8d6e16a149708fb8a08c5c80d431` aprobada en Atlas.
El usuario autorizó integrar los aprobados antes del ZIP; se conserva
la valoración sin elevarla a honores. Los comentarios nuevos permanecen
en la app para una hija posterior, a cargo del carril de Claude.

Integración 25/10: escena 116, versión `4767d7d89689440084a12cf7beeab8b9` aprobada en Atlas.
El usuario autorizó integrar los aprobados antes del ZIP; se conserva
la valoración sin elevarla a honores. Los comentarios nuevos permanecen
en la app para una hija posterior, a cargo del carril de Claude.

Integración 25/10: escena 115, versión `edd1e953dd7141ad931a43429c621a42` aprobada en Atlas.
El usuario autorizó integrar los aprobados antes del ZIP; se conserva
la valoración sin elevarla a honores. Los comentarios nuevos permanecen
en la app para una hija posterior, a cargo del carril de Claude.

Escena 102, versión `dd5a82c201a84ca29180026bc7cf064a`, aprobada 25/10 e
integrada por autorización directa del usuario antes del ZIP (sin elevar su
valoración a honores). En el encuentro entre una pared que gira y su
continuación diagonal se conserva primero la curva opaca original; el donante
recto solo continúa después del encuentro. Los cuatro pilares cortados por el
mapa siguen su espiral de cinco filas hasta salir del viewport, sin boca plana
suspendida. Centro GBA intacto. Las tres nubes nuevas sobre los pilares cortos
quedan para la siguiente hija; no se altera la aprobación de esta versión.

Criterio 25/10, escena 115: la madera mantiene las
cuatro filas de cada tabla y la fase diagonal al continuar fuera del mapa.
Las escotillas repiten módulos completos en su retícula escalonada, no tiras
recortadas; se descarta una copia parcial cuyo centro pertenece a una celda
vacía del mapa original. No completar piezas atravesando el centro GBA.

Propuesta 25/10, escena 116 (pendiente de valoración): continuar por separado
el muro posterior y la baranda delantera, trasladando módulos completos de
ventanas, molduras y faroles sobre sus diagonales. El casco rojo continúa bajo
la baranda; no sustituir el exterior por suelo ni repetir los faroles en vertical.
La esquina plana del fondo mantiene su fase propia al girar el muro derecho.

Propuesta 25/10, escena 117 (valoración pendiente): cerrar la tapa cortada de
una caja con otra tapa equivalente de su propio fondo. Cuando el donante cruza
el mapa, conservar el reflejo y la fase por píxel de su punta completa.
Para la pared diagonal, escoger una banda cuyo encuentro con el suelo esté
entero dentro del mapa y una muestra superior completamente opaca; una banda
cortada por el borde genera escalones y fragmentos repetidos.

Propuesta 25/10, escena 118 (valoración pendiente): las vigas transversales y
el casco siguen diagonales opuestas y se muestrean por separado. Continuar el
casco, la filigrana y los módulos completos de cañón/cuerda, conservando el grosor
de ambas vigas y un solo mástil en su posición. Una cuerda que cruza el antiguo
límite rectangular del mapa debe terminar completa en la vista añadida.

1. **Solo píxeles del juego.** Nada se dibuja nuevo: se reutilizan los tiles de
   la propia sala/escena, volteados o repetidos (bits 0x400 hflip / 0x800 vflip
   de la entrada). Si de verdad no hay tile adecuado, se puede dibujar uno, con
   el mismo estilo (el usuario lo autorizó el 18/10).
2. **Un borde natural cierra el mundo.** Donde la sala termina en corte recto
   se continúa con el *propio borde* de la sala (su festón inferior) volteado,
   vacío fuera y muro dentro. Nunca un corte recto ni un zigzag inventado.
3. **Nunca repetir adornos únicos.** Marcos de ventana, relieves y figuras no se
   copian ni se cortan por la mitad: el espacio nuevo se rellena con la pared
   lisa de esa columna. Mejora aprobada con honores en 109 (21/10): sí se pueden
   intercalar piezas ambientales completas del propio escenario (puestos y
   grupos de jarrones), con separaciones variadas y anclaje correcto al suelo.
   Primero reparar el borde; después decorar. Sin nuevos objetos de juego.
4. **Pared lisa = capa base.** En estas salas BG3 lleva la pared de rayas y
   BG2 solo los adornos; fuera de la sala: BG2 vacío (0) y BG3 = el tile de
   BG3 más repetido de esa columna (`MapPlainBaseTile`). Elegir por frecuencia
   dentro de la columna, no "el primer tile".
5. **Detección genérica, no por número de sala.** Buscar en los datos de la
   sala la tira que cumple una forma (24 tiles seguidos con borde arriba y
   vacío justo debajo) en vez de fijar coordenadas; así sirve a otras salas
   del mismo tileset. Si no se encuentra, no se toca nada (comportamiento
   original).
6. **GBA intacta.** Todo vive bajo `PLATFORM_PC` y solo fuera de la ventana
   original; se puede apagar (`KHCOM_TOPCLOSE=0`).

## Método (rápido)
Workflow completo (tiles, píxeles, pixel art, pruebas en x3 entero): WORKFLOW_ARTE_AMPLIO.md.


1. Reproducir la sala exacta y capturar (ver PRUEBAS_VISUALES.md).
2. Volcar los tiles: `KHCOM_ROOMDUMP=1` → `roomdump.txt` (BG1/2/3) y
   `python port/tools/room_ascii.py roomdump.txt --bg 1 --rows .. --cols ..`
   para ver la forma del borde, las tiras repetibles (`--strip`) y qué tiles
   son comunes.
3. Probar la idea con variables de entorno antes de fijarla; recortar y ampliar
   la zona; comparar con lo que marcó el usuario (azul = quitar, naranja = usar).
4. Entregar copia jugable + .bat; esperar aprobación; solo entonces dejarlo
   activo por defecto y anotarlo aquí.

## Errores que el usuario descartó

- Continuar el muro hacia arriba repitiendo la fila superior (eco de textura).
- Zigzag con las escaleras de las esquinas (piezas que no encajan).
- Elegir como "pared lisa" una celda con marco de ventana: se ve media ventana
  flotando. La pared lisa se decide por tile y por frecuencia, en BG3.

## Manejo de PR y de la cola (24/10, para no repetir errores)

- **Integrar = merge + cola + verificación, siempre juntos.** Después de cada merge: build de PC desde main, combate de
  prueba, una sala y el launcher; luego marcar la pieza `integrado` en la cola online con el número de PR y la
  aprobación real. Un merge sin la cola actualizada deja la pieza como `pr_abierto` y otro agente la retoma.
- **La aprobación se registra tal cual fue:** «aprobado en la app (versión …)» o «orden del usuario en el chat
  (fecha): integrar todos los PR». Nunca inventar una aprobación en la app que no existió.
- **PR frenado por el usuario** (p. ej. #71: recuadro negro, escalones, hueco marrón, recetas 115-118 sin aprobar) no
  entra con un «integra todos» genérico: se pregunta una vez nombrándolo.
- **Conflictos en LOGICA_APROBADOS.md** entre PR de arte: son secciones nuevas en el mismo lugar; se conservan las dos
  y se vuelve a mezclar main si otro PR entró mientras tanto.
- **Permiso de integrador de la cola** (`integrator-acquire`): guardar el token que devuelve SIN `--quiet` (con
  `--quiet` no se ve); si se perdió, `recover-integrator` con el mismo dueño y volver a pedirlo; soltarlo al terminar.

## Aprobados

- 17: borde superior de la sala de Riku (Hollow Bastion B1, sala 3): aprobado
  totalmente el 18/10 (`src/map/map_cell.c`: `MapFindEdgeStrip`,
  `MapTopEdgeTile`, `MapPlainBaseTile`).
- 061 hielo, 074 Monstro y 057 playa (18/10) y 004: **aprobadas antes de tiempo** (usuario 24/10). Ya no son
  modelo; ver «Aprobaciones tempranas corregidas» abajo. Guard Armor y las ventanas 1-17 siguen valiendo
  (REPARTO.md, ESCENAS_REVISION_AMPLIA.md).

- 59 retratos: aprobado el 18/10. La vista amplia exponía las posiciones de espera
  X=449/-299 antes de aplicar la silueta. Ocultar la espera en `msgface_2` y
  recortar su entrada/salida al área original de la ventana desplazada. Aplicar
  en la función común, sin excepciones por personaje/escena. Commit `acd85c818`;
  detalle en RETRATOS_59_LOGICA.md.

- 60 título: aprobado el 18/10. Prolongar las filas planas de BG0 y del OBJ
  del pie de brillo, con paleta viva, mezcla alpha y alineación PixelEdge.
  No tocar letras, ilustración ni píxeles del rectángulo GBA. Detalle:
  TITULO_60_LOGICA.md; commit `6039fc04d`.

- 69 lógica mapa GBA: aprobado el 19/10. Las mejoras propias de ReKHCoM que
  cambian lógica del juego van como opción en Trucos → Modo debug, apagadas por
  defecto, con prefijo "[ReKHCoM]" y un panel sobre el arte de Sora que las
  explica con diagrama, en los 6 idiomas. Fuera de eso, la lógica del juego no
  se toca (63: nada de copias propias de objetos). Commit `8a74f004b`.

- Guard Armor, escena 008 (las piezas caen y se arman antes del combate): aprobado "con
  honores" el 20/10. El patio del combate cubre también el evento: la extensión se elige
  por los cuatro mapas del fondo, sin exigir el modo combate (`port_ppu.c`, `guardPatio`).
  Lección: buscar todas las apariciones de un fondo (WORKFLOW_ARTE_AMPLIO.md, paso 1).

- Agrabah, calle de Aladdin, escenas 107/108: aprobada **«con honores»** el
  21/10. Continuar por separado la diagonal de calle y la de las fachadas de
  las azoteas; entre ambas, techo liso del propio mapa. Conservar todas las
  filas del bordillo, incluidos los medios tiles de cada paridad. No repetir
  ventanas, jarrones, puestos ni esquinas. Identificar los mapas por sus datos
  descomprimidos (EU) o directos (US/JP), con paleta activa. Receta en
  `AGRABAH_107_REVISION.md`. Codex commitea; Claude integra.

- Agrabah 109: aprobada **con honores** el 21/10. Borde diagonal completado con una tira donante intacta; puestos y grupos de jarrones completos intercalados con separaciones variadas, como arte de fondo y con paletas BG/OBJ activas. GBA y OAM intactos. Receta en AGRABAH_109_REVISION.md; mejoras incorporadas a WORKFLOW_ARTE_AMPLIO.md para la tanda Preliminar V1.

- Jafar, sala de lava (Preliminar V2): aprobada el 24/10 ("se veía genial, quedaba muy bueno el efecto"). Encendida
  para todos; `KHCOM_JAFAR_WIDE=0` la apaga. Pared, columnas, orilla y lava siguen por sus períodos de tiles; los
  pilares móviles solo en la sala; un solo genio. Receta en BATTLE_SCENERY_TECHNIQUE.md punto 12 (`port_jafar.c`).
  Corrección del usuario 25/10: mantener el efecto de extensión/reflejo del escenario, excepto Jafar.
  Su copia se fija al desplazamiento de su cuerpo (cuadrante 768,256), nunca al centro de la cámara.
  Reenlazar sus buffers mutables conserva el origen del mapa; el desfase de pose entre buffer y VRAM no
  deshabilita la extensión. La mejora del patrón inferior sigue pendiente y no se altera en este arreglo.

- Integración (22/10): 107/108 y 109 encendidas por defecto para los jugadores; `KHCOM_AGRABAH_WIDE=0` /
  `KHCOM_AGRABAH109_WIDE=0` las apagan en las pruebas. Regla: lo aprobado (o con honores) va encendido, nunca detrás
  de una variable de prueba.


## Tablets y plegables (aprobado 24/10)

En pantallas más altas que 16:9 la vista Auto usa las filas de una pantalla 16:9 del mismo ancho (`PortConfigAutoView`,
port_config.c): más grande, sin franja negra sobre el fondo, el HUD cerca de la acción. `KHCOM_TALL_FIT=0` lo apaga.

## Revisiones valoradas el 24/10 (app, 13:06–13:12 Chile)

La valoración se conserva por versión; una aprobación anterior no aprueba una hija nueva.
Las recetas ya están en main. La activación para jugadores sigue la referencia con honores de la app.

| Pieza / versión | Valoración humana | Criterio que sí quedó aceptado |
|---|---|---|
| 044 / `0ac326ad582f4214b308a5d0c1bc6629` | Aprobada | Continuar las casas y la arcada; conservar una sola torre del reloj y quitar la tira suelta de fachada. |
| 045 / `2f0e9dee739642468bded660684f5355` | Aprobada | Césped de la extensión con el tono de la sombra del bosque; completar el pino cortado con su otra mitad. |
| 053 / `553e59d173b14e00bc61c1d513188719` | Con honores | Colocar matas completas del propio mapa sobre hierba lisa, separadas y con densidad parecida; ninguna mata partida por el donante. |
| 054 / `18f40e19e5814241a966b3348a542977` | Con honores | Prolongar el muelle por su diagonal hacia abajo a la izquierda, repitiendo dos pilotes completos por tramo. |
| 098 / `de7e4e0b84f346c3a5cb9fd905010b50` | Aprobada | Cada muro y el ajedrezado superior siguen su propia diagonal: papel y barandilla a la izquierda, pilar y celosía a la derecha. |
| 104 / `52df9ff43f4e48b4a722240c723bc2f8` | Aprobada | Donantes con los brillos enteros; algas prolongadas y cerradas; suelo y borde continuos sin repetir el jarrón. |
| Darkside 03 r6 / `2938cbbb50a345088e08d0365dd0b9f3` | Aprobada | Continuar torso y mechones/ala cortados; tramado solo en lo añadido y píxeles originales intactos. |

Aplicación al siguiente caso: medir el período y dirección de cada estructura; comprobar que el donante contiene
la pieza entera, también encima del corte y en la última fila ocupada. No trasladar las coordenadas de un fondo
a otro. Estas valoraciones documentan casos aceptados, no una aprobación automática de recetas futuras.

## Aprobaciones tempranas corregidas (usuario 24/10, 13:17–13:20 Chile)

El usuario dio por aprobadas 004/057/061/074 demasiado pronto y volvió a comentarlas sobre la versión integrada.
Lo que se había aprendido de ellas no basta; estas son las reglas corregidas (valen para toda escena nueva):

| Escena / versión comentada | Comentario | Regla corregida |
|---|---|---|
| 004 / `22fc4ea176864111880e02b851a58445` | «falta acabar patrón hacia la derecha» | Un patrón sigue **hasta el borde de la pantalla en los dos lados**; no basta con cerrar el lado más visible. Comprobar el ancho máximo (3840) en ambos márgenes. 004 (r3c): perfil propio; la pared repite con (6,-3) a lo largo de su base (periodo 6 medido en el mapa), el suelo 8x4; alfombra, cajas y manchas quedan únicas; el borde del zócalo se mide por tile (2y+x). |
| 057 / `e49d2f465f9346d28ba013d9a81de938` | «el patrón no se sigue hacia arriba bien» (4 nubes); «pelones a los que falta la mitad derecha del pasto»; «mejorar derecha del pasto» | Repetir **hacia arriba** con el período vertical medido del propio patrón, no con la fila superior. Cada mata de pasto entera (las dos mitades): el donante debe contenerla completa, como 053. El lado derecho con la misma estructura que el izquierdo. |
| 061 / `f31927d3f7014eb3a51e52fcf7323003` | «falta un cachito»; «falta terminar el patrón de pasillo» | Sin huecos: revisar cada esquina y unión entre la extensión y el mapa. Un pasillo sigue con su propio período hasta el borde, sin cortarse antes. |
| 074 / `303416c343704456aea87be42f0508bf` | «el patrón no está bien, creo que había una versión mejorada, mejorar» | Antes de rehacer, buscar en la app las versiones anteriores de la pieza y partir de la mejor valorada; el patrón debe repetir su período real. |

Regla general que sale de las cuatro: medir el período de cada estructura en **horizontal y vertical**, extenderla
**en todas las direcciones** hasta los bordes de la pantalla más ancha, y no dar por terminado un lado sin revisar
los demás. Las escenas hermanas que usaron estas recetas se revisan con estas reglas al tocarlas.


## Comentarios nuevos sobre arenas y Darkside 06 (usuario 24/10, 05:33-13:07 Chile; worker claude-sonnet-w3)

| Pieza / versión comentada | Comentario | Regla |
|---|---|---|
| Darkside arena / `85b593a92f164922966b64436ccf06ab` | «no está completando la arena» (huecos negros entre copias a 32:9; «abajo también le falta dibujo») | La entrega se captura con **todas** las capas V2 de la arena encendidas: `arena_before_shots.py --v2` ahora activa también `KHCOM_DARKSIDE_BEACH=1` (la playa de `port_darkside.c`). Antes de comentar un hueco de arena, comprobar que la captura se hizo con las variables de revisión completas. |
| Darkside arena / `14ffdd662e9d4fde9316e74c7f1e8208` | «no veo los sprites y animaciones con fadeout que hicimos, el fondo esta bien supongo» | Una lámina de poses no prueba que estén conectadas al juego. El renderer debe seleccionar por slot la pose revisada (agachado, brazo alto, garra, inclinado, puñetazo y alcanzar), con el ancho real de su matriz 4bpp y su tramado de disolución. Mantener el fondo ya aceptado; mostrar capturas reales de animación, no volver a entregar sólo la pose de pie. El puñetazo r7 conserva el desplazamiento documentado de 11 px respecto de r6. |
| Darkside arena / `433401942ccd4f4284514144e9d77364` | «mejorar fading» en el brazo que alcanza | La disolución no debe leerse como dientes horizontales. En esa pose, cerrar sólo huecos pequeños encerrados en la silueta añadida y distribuir la cobertura en dos dimensiones, con ruido espacial fijo y caída suave hasta transparente. El patrón permanece estable entre fotogramas; las otras poses revisadas y todos los píxeles originales se conservan. |
| Darkside 06 r6 / `ad293ae9c5324daf883db049dd30af46` | «ahora quedó demasiado arriba» | Mover una pieza hasta la posición literal de una nube anterior (23 px) puede pasarse: r7 la baja 11 px (`darkside_revise.py --round r7`). Ante un ajuste de posición pedir/medir el punto medio, no el extremo. |
| Oogie arena / `b7df95116b0543719423d2b29c7aa17f` | «no debería seguir un patrón, sino dibujar como pixel artist las puntas que faltan en el GBA de los botones del suelo, a ambos lados; dejar en penumbras el resto como un vacío»; los fondos y otro patrón «quedaron geniales» | Pendiente (bloqueado, arte nuevo): los botones del suelo se completan dibujando sus puntas faltantes en pixel art, no repitiendo patrón; fuera de eso, penumbra/vacío. Los patrones de fondo marcados «geniales» se conservan. |
| Trickmaster arena / `bcf3aec4ee7f40748959a87960d4ed5d` | «solo debe haber una chimenea en el patrón como en la escena preliminar» | Conservar la chimenea del mapa original y prolongar únicamente los paneles limpios de pared en los márgenes. El mapa de 256 px contiene una chimenea con dos bocas, no dos chimeneas: repetir el mapa duplica todo ese objeto. El suelo conserva un único punto de fuga, sin reiniciar la perspectiva cada 256 px. |

| Oogie arena / `7420dbb247c440e2b70a1285ce21dbe8` | Alfombra «solo rojo», completar una máscara a la izquierda y un fantasma a la derecha, luego abismo; abajo dar profundidad | Una figura por borde, nunca repetir el campo verde. La propuesta completa el recorte con un reflejo limitado de 32 px, estrecha sus dos puntas inferiores y las oscurece hacia el vacío. Bajo el mapa la alfombra continúa roja, sin estirar el antebrazo. El fondo superior y el centro GBA se conservan. Evaluar la silueta de la propuesta en la captura real; un cambio de algoritmo no basta para dar por aprobado el arte. |
| Trickmaster arena / `bcf3aec4ee7f40748959a87960d4ed5d` | «solo debe haber una chimenea en el patrón como en la escena preliminar» | Pendiente de aclarar cuál se conserva (el mapa trae dos chimeneas por período de 256 px). |
| Maléfica 01 / `25aa0ffe25b14be5855a107bd882b6f6` | «terminar de dibujar a Maléfica hasta la cola; ver si está en los archivos o dibujar como artista pixel art» | Pendiente (bloqueado, arte nuevo): buscar el resto del sprite en la ROM; si no existe, dibujar. |
| Maléfica arena / `ae681fffa79a4feea3b4ea1ef137a512` | «arena bien» (aprobada); sprites se comentan por separado | Sin cambios: la arena de Maléfica queda tal cual. |

## Escena 090: reja recta y montañas completas (respuesta Codex, 24/10)

Referencia humana: `0551fbab97804de9802ed1b48160e278`, envio `20261024-130932-cd374d`.
Las nubes 1/2 piden la reja recta del propio fondo; 3/5 completar montañas; 4 señala el patron correcto.
Correccion: no repetir la esquina diagonal; usar un poste completo horizontal (columnas 40..43),
sin el remate unico del porton ni estela lunar, y mamposteria separada sin lapidas (46..47).
Montañas: tres crestas en 32 columnas, incluyendo todas sus sombras y excluyendo remates del porton.
El lado izquierdo conserva su diagonal; luna y puerta siguen unicas. Hija `37a6f9b61f0e427988c96890ce9b4f09`.
Propuesta preliminar pendiente de valoracion: no hereda honores. Evidencia en ESCENA090_REVISION_CODEX.md.

## Medición: workers vs. trabajo solo (24-25/10/2026)

| Modo | Agentes | Piezas de arte entregadas a la app | Aprobadas | Costo aprox. |
|---|---|---|---|---|
| Solo (Claude, 24/10 14:58-16:24, ~1,5 h) | 1 | 12 hijas (090, 044, 045, 053, 054, 087, 095, 098, 099, 102, 104, 110) | 044, 045, 053★, 054★, 098, 104 aprobadas | 1 contexto |
| Ronda 1 workers | 3 | 1 (Darkside 06 + arena, #70) + 112/120 sin imagen (#71) | 0 aún | ~25 min de preparación cada uno |
| Ronda 2 | 3 | 0 (murieron por límite de sesión) | 0 | todo perdido |
| Ronda 3 + limpiador | 4 | 1 (004, #75) + imágenes de 112/120 | 0 aún | ~380 k tokens |
| Ronda 4 (1 worker) | 1 | 1 (087, #76); 17 devueltas como pregunta sin intento | 0 | ~390 k tokens |
| Ronda 5 | 1 | 0 (detenido) | 0 | — |

Resultado: **11 agentes de worker → 3 piezas entregadas, 0 aprobadas**; solo → 12 entregadas y 6 aprobadas en ~1,5 h.
Es una **regresión clara en tiempo y en esfuerzo** (≈1 pieza por ~250 k tokens contra ≈12 piezas en un contexto).

Por qué:
1. **Preparación repetida**: cada worker reconstruía worktree, ROMs y build (5-25 min) antes de tocar arte.
2. **Contexto perdido**: el worker no tiene la lógica aprobada en la cabeza; lee el brief, ve un comentario «vago» y lo libera.
   El agente que viene trabajando las escenas sí entiende «mejorar patrón superior» mirando la imagen.
3. **Modelo y esfuerzo bajos** para un trabajo de criterio visual (Sonnet bajo): bueno para mecánica, malo para arte.
4. **Fricción de herramientas** (rutas, `prelim_shot.sh` a `build/pc`, `next` en bucle, `--title`/`--version`, ninja `-f`) consumió
   rondas enteras; se arreglaba entre rondas en vez de antes.
5. **Límite de sesión**: muchos agentes a la vez agotaron el cupo y mataron la ronda 2 completa.
6. **Sin obligación de intentar**: la regla de liberar «si es ambigua» se volvió la salida por defecto.

Regla desde ahora: el arte lo hace el agente principal (con contexto), en serie, con la base del generador Python y su
mejora propia; otros agentes (Codex) toman piezas en paralelo por la cola. Los workers solo para tareas mecánicas
(capturas, baterías, limpieza), nunca para decidir arte.

## Escena 095: comentario posterior sobre laberinto y castillo (24/10, propuesta Codex)

Referencia humana: versión `063184fedd8043a492e2fe4b5ea34b25`, envío `20261024-130959-c53dec`; nubes
`c97b37ea-0aeb-48ad-a8bc-5f8975e97205` (prolijidad del patrón) y `d6b9f5ce-4dc7-45bf-9daa-9f2357f6849b`
(patrón y castillo único). La receta anterior no constituye un modelo aprobado.

El laberinto del fondo y los setos del primer plano necesitan donantes separados: el plano lejano no se
sesga con la diagonal del seto. El módulo de seto izquierdo conserva sus doce columnas; ampliarlo a dieciséis
introduce el hueco de la puerta. La derecha debe reutilizar un módulo completo reflejado a nivel de píxel,
no una tira que copie parte del trono ni cuatro columnas que produzcan escalones. El horizonte añadido
repite una ladera sin la silueta del castillo; el dibujo original dentro del mapa y el rectángulo GBA se conservan.
Esta entrega es una propuesta pendiente de valoración; no hereda honores ni habilita arte nuevo para jugadores.

## Escena 099: techo, propuesta de Codex (24/10)

La nube bab8281c-e1c9-4e01-b894-a3a05e339e8b sobre d3e87dca65eb4e9fb10535b3e731419d pide mejorar el patrón superior y valora positivamente el suelo. No trasladar esa aprobación al techo ni cambiar el suelo. El friso mide 28 píxeles por casilla: usar una pareja completa de 56 píxeles, aunque no coincida con la rejilla de tiles, y conservar su fase en las dos uniones. Propuesta pendiente de valoración: completar arriba filas alternadas de casillas con el mismo donante; el mapa original queda intacto. No se habilita para jugadores sin honores de esta versión.

## Agrabah 107/108: subida del Genio (comentario posterior, 24/10)

El usuario señala que, aunque la calle tiene honores, al aparecer el Genio la cámara sube y los pilares
rosas y la ventana superior terminan en un corte. Esa aprobación no certifica el margen vertical.
Regla corregida: verificar los extremos de la cámara del evento compartido, continuar cada pilar en
su propia columna con su período vertical (4 filas), y completar únicamente la ventana que ya queda
cortada, con su donante entero, sin repetir otras ventanas. Rectángulo GBA intacto.
La propuesta se ve con KHCOM_PRELIMINAR_V1=1 y espera valoración propia; no hereda honores.

## Comentarios nuevos 24/10 (112, 120; sin valorar)

- 112 y 120 (usuario): los personajes que esperan fuera de la cámara GBA (Aladdin, Pepito) no deben verse en el margen
  hasta entrar en el rectángulo original. Es la regla de la escena 097 (`sceneActorFrame`), ahora también para las
  recetas Preliminar V1 de 112 y 120. Las demás nubes (patrón/pilares/puerta de 112, estructura y muro de 120) siguen
  abiertas: requieren estudio de tiles por escena.
- Cierre del PR #71 (24/10): entra la regla de actores y la vista previa de 112 y 120 (`ready=1`); 115-118
  vuelven a `ready=0` hasta tener su propia revisión. Los defectos de 112/120 (recuadro negro junto a la puerta,
  escalones en el muro, hueco marrón en 120) quedan en la cola de arte; nada de esto llega a jugadores sin aprobación.
- Comprobar que `port_preliminar_v1_data.inc` lleva `ready=1` en la escena: con `ready=0` la receta no se aplica ni
  con `KHCOM_PRELIMINAR_V1` y la captura sale sin extensión.

### Oogie — dibujar figuras, no reflejar (feedback 25/10)
Padre `aa913fe773c4446eb5412c3115e222c7`, nubes
`63ebc9c6-8491-46b9-8a60-36d33c92eb8f` y `e08c24c4-0eb3-4ea7-8e50-a9e9f3fd53ee`:
completar cada fantasma con contorno propio en índices nativos, sin copiar ni
reflejar su mitad. La bajada prolonga verde liso a ambos lados, se estrecha en
perspectiva y oscurece hacia el abismo; no arrastra figuras al muro. La hija
`e90c2d6d475447458a05db85276db6bb` responde ambas nubes y los 17 trazos.
Propuesta offline editable en `port/art/proposals/oogie-no-mirror/`, sin activar
ni cambiar valoraciones. Falta captura runtime cuando se asigne turno RAM.
