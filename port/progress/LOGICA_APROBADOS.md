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
| 004 / `22fc4ea176864111880e02b851a58445` | «falta acabar patrón hacia la derecha» | Un patrón sigue **hasta el borde de la pantalla en los dos lados**; no basta con cerrar el lado más visible. Comprobar el ancho máximo (3840) en ambos márgenes. |
| 057 / `e49d2f465f9346d28ba013d9a81de938` | «el patrón no se sigue hacia arriba bien» (4 nubes); «pelones a los que falta la mitad derecha del pasto»; «mejorar derecha del pasto» | Repetir **hacia arriba** con el período vertical medido del propio patrón, no con la fila superior. Cada mata de pasto entera (las dos mitades): el donante debe contenerla completa, como 053. El lado derecho con la misma estructura que el izquierdo. |
| 061 / `f31927d3f7014eb3a51e52fcf7323003` | «falta un cachito»; «falta terminar el patrón de pasillo» | Sin huecos: revisar cada esquina y unión entre la extensión y el mapa. Un pasillo sigue con su propio período hasta el borde, sin cortarse antes. |
| 074 / `303416c343704456aea87be42f0508bf` | «el patrón no está bien, creo que había una versión mejorada, mejorar» | Antes de rehacer, buscar en la app las versiones anteriores de la pieza y partir de la mejor valorada; el patrón debe repetir su período real. |

Regla general que sale de las cuatro: medir el período de cada estructura en **horizontal y vertical**, extenderla
**en todas las direcciones** hasta los bordes de la pantalla más ancha, y no dar por terminado un lado sin revisar
los demás. Las escenas hermanas que usaron estas recetas se revisan con estas reglas al tocarlas.


## Comentarios nuevos 24/10 (112, 120; sin valorar)

- 112 y 120 (usuario): los personajes que esperan fuera de la cámara GBA (Aladdin, Pepito) no deben verse en el margen
  hasta entrar en el rectángulo original. Es la regla de la escena 097 (`sceneActorFrame`), ahora también para las
  recetas Preliminar V1 de 112 y 120. Las demás nubes (patrón/pilares/puerta de 112, estructura y muro de 120) siguen
  abiertas: requieren estudio de tiles por escena.
- Comprobar que `port_preliminar_v1_data.inc` lleva `ready=1` en la escena: con `ready=0` la receta no se aplica ni
  con `KHCOM_PRELIMINAR_V1` y la captura sale sin extensión.
