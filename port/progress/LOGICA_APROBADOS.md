# Lógica de los arreglos aprobados por el usuario (vista amplia)

Reglas que se repiten en todo lo que el usuario ha aprobado. Aplicarlas antes
de proponer algo nuevo; ante un caso nuevo, derivar la solución de ellas.

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
- 061 hielo y 074 Monstro: aprobadas (18/10). 057 playa: aprobada con mejoras
  (igualar la estructura de tiles del lado derecho-superior con la del izquierdo;
  no cortar motivos de pasto por la mitad).
- Anteriores (márgenes de ventanas 1-17, escenas 004/057/061/074, Guard Armor):
  ver REPARTO.md y ESCENAS_REVISION_AMPLIA.md; mismos principios 1-3.

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

- Coliseo 121, Cloud/Hércules/Phil: aprobada **con honores** el 24/10. Literal del usuario: «como esa ultima
  de cloud con hercules, desde ya la apruebo con honores». Versión corregida `1ee40e4e6`, captura JP frame300
  3440x1440. Conservar tres planos de mampostería, frisos por paridad, fuste y ventanas completadas desde
  sus donantes originales; desgaste único. Evidencia/receta en ESCENA121_REVISION.md. Adjunto humano
  preservado privado en `build/art-review/source-studies/aprobacion-121-adjunto-usuario.png`.
  Aprobación registrada por versión: una propuesta posterior no la revoca. Integración aislada pendiente;
  122 y borrador124 no quedan aprobados por esta aprobación.

- Jafar, sala de lava (Preliminar V2): aprobada el 24/10 ("se veía genial, quedaba muy bueno el efecto"). Encendida
  para todos; `KHCOM_JAFAR_WIDE=0` la apaga. Pared, columnas, orilla y lava siguen por sus períodos de tiles; los
  pilares móviles solo en la sala; un solo genio. Receta en BATTLE_SCENERY_TECHNIQUE.md punto 12 (`port_jafar.c`).

- Integración (22/10): 107/108 y 109 encendidas por defecto para los jugadores; `KHCOM_AGRABAH_WIDE=0` /
  `KHCOM_AGRABAH109_WIDE=0` las apagan en las pruebas. Regla: lo aprobado (o con honores) va encendido, nunca detrás
  de una variable de prueba.

