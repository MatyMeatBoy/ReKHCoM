# Continuar escenarios desde sus tiles y píxeles originales

Método aceptado por el usuario para Guard Armor, 18/10/2026. La referencia
visual aprobada es el prototipo del patio y después su integración real en
16:9 y 32:9. No trasladar automáticamente sus constantes a otro escenario.

1. Decodificar localmente los mapas, tiles y paletas originales y ubicar el
   rectángulo realmente ilustrado. Un mapa declarado de 512×512 puede contener
   grandes zonas transparentes; repetir sus dimensiones duplica la escena y
   sus huecos, no la continúa.
2. Separar zonas únicas (torre, escalera, mástil, jefe, plataformas) de bandas
   repetibles (fachada, tejado, arcos, pavimento). Conservar las primeras y
   continuar cada banda con el período y dirección de su propio dibujo.
3. Para suelos en perspectiva, trabajar fila por fila con los colores y
   límites de las celdas del suelo original. Guard Armor busca dos colores
   dominantes en cada fila y toma pares de celdas cerca de cada borde; así no
   pega un rectángulo de suelo de tamaño fijo a todas las alturas.
4. Por debajo del frente ilustrado, continuar la anchura creciente de las
   baldosas con un dibujo en coordenadas del mundo. La continuación del patio
   usa dos índices de color del propio pavimento, no una imagen filtrada.
5. Muestrear **índices de paleta** y resolverlos con la paleta hardware activa
   en cada frame. Los fundidos y efectos de color del juego se conservan.
   Nunca hornear una captura con sus colores actuales como textura final.
6. Aplicar la extensión en el renderizador, únicamente fuera de los 240×160
   originales y solo al mapa/capa identificados. No ampliar colisiones,
   duplicar enemigos ni modificar la lógica de la cámara o del combate.
7. Comparar el mismo ejecutable con la extensión activada/desactivada:
   región GBA idéntica, vista Original idéntica, otros mapas idénticos, y
   capturas de la cámara en varias posiciones a 16:9 y 32:9. Revisar después
   las animaciones particulares de cada jefe y los personajes/regiones.
8. Guardar cada variante aceptada en un commit con su receta, límites y
   evidencia privada. No introducir en Git ROMs, capturas extraídas ni saves.

Implementación de referencia: `port/src/port_guard_armor.c` y su llamada desde
`DrawTextBg` en `port_ppu.c`. Identificación por los cuatro bloques concretos
del mapa, sin bandera persistente que pueda quedar activa en otra pelea.
El muestreo usa los tiles de la ROM instalada del usuario; no añade arte al
paquete. Los datos derivados de filas son caché reconstruible, no savestate.

Prueba: `python port/tools/test_guard_armor_backdrop.py`. El harness
`KHCOM_NO_GUARD_PATIO=1` permite el antes/después en carpetas aisladas.

## Resultado de Guard Armor

- Aceptación visual explícita del usuario: «queda como aceptada, integrar».
- Windows EU/US/JP compilan.
- 13/13 fixtures de comparación: Sora EU/US/JP a 16:9 y 32:9, Riku EU en
  ambos aspectos, Original EU/US/JP, Hook y Monstro. Tres frames por caso;
  cero diferencias en el rectángulo GBA. Original y las otras dos arenas
  permanecen idénticas en toda la imagen.
- Capturas de juego: `build/codex-evidence/guard-patio-preview/`.
- Comparaciones: `build/codex-evidence/guard-patio-tests/report.json`.
- No equivale a una partida completa ni a todas las escalas posibles.

## Aplicación a próximas escenas

Mantener las escenas que el usuario ya considera buenas. En Alicia solo
falta el suelo inferior: no reconstruir paredes ni chimenea. En Darkside,
Úrsula y Maléfica separar especialmente la capa del jefe de la del escenario.
En Jafar preservar pilares animados y límites jugables. Revisar cada propuesta
antes de adoptar sus períodos: la receta es el proceso, no las constantes.

## Preliminar V2 de las arenas de jefes (23/10 noche) — lógica aprendida

Todo detrás de `KHCOM_ARENAS_V2=1` (apagado para los jugadores hasta la aprobación del usuario); capturas antes /
después a x3 con `py -3 port/tools/arena_before_shots.py [--v2] [--only arena]` (`build/arena/v2-before|v2-after`).
Comprobación de cada arena: el rectángulo GBA idéntico píxel a píxel antes/después en 16:9 y 32:9.

1. **Medir antes de dibujar.** `bgdecode.py` (scratchpad; receta: los símbolos de `build/eu/pc/rom_symbols.s`, mapa
   de 32x32 entradas por bloque, tiles 4bpp, paleta 16 colores por entrada) muestra qué parte del mapa está
   dibujada: Maléfica solo x 64..447; Trickmaster el mapa entero; Úrsula todo el ancho.
2. **Perspectivas: geometría del propio cartucho.** El ajedrez de Trickmaster es un suelo en perspectiva real: las
   columnas se ensanchan linealmente con y hacia un punto de fuga (y = 33,3) y las filas son equiespaciadas en
   profundidad (1/(y - fuga) baja 0,000564 por fila). Medido de los cambios de color de sus filas; repetir filas no
   sirve (las cuadrículas crecen hacia el frente).
3. **Bandas regulares: el período de su dibujo.** Los arcos de Maléfica se repiten cada 72 px (arco + pilar con
   gárgola): fuera de lo dibujado cada fila sigue con una bahía completa propia; arriba el cielo liso, abajo el
   borde de piedra (sus 32 filas, las últimas 16 envueltas a la fila 0 del mapa).
4. **Lo que el mapa ya trae debajo.** Úrsula: las primeras 64 filas del mapa son lo que hay bajo su suelo (borde de
   pinchos y el resplandor rojo); usarlas es arte del cartucho, no inventado.
5. **Una sola criatura.** El dragón y Úrsula viven en una capa propia (sus mapas por pose en un bloque; los otros
   tres vacíos): fuera de la imagen solo se dibuja la copia más cercana a ella (`PortMaleficentDragonVisible`); antes
   la repetición lateral de la capa mostraba dos o tres.
6. **No todo es fondo.** El lomo de roca de Úrsula son dos sprites (mitades izquierda y derecha en x 128 y 384 del
   mundo, `task_bos_ursula_border_2`): se continúa dentro de la función del juego, dibujando las mismas mitades un
   período del fondo (512) a cada lado, solo en la V2.
7. **Arriba no siempre es lo mismo que abajo.** En Trickmaster el ciclo vertical del mapa ya pone suelo por encima de
   la imagen: la extensión se limita a debajo de ella (`gy >= 160`).
8. Pendiente de arte nuevo (como Darkside): el ala del dragón se corta en el borde de su bloque.
9. **Leer el mapa en tiles, no en píxeles** (Oogie, 24/10): la sala (30 x 35 tiles, x 256..495, y 192..471) está
   armada simétrica y con pocas piezas: columnas de 2 tiles, ladrillo liso en dos filas de tiles alternadas
   (filas 232 y 256), guirnaldas cada 64 px, calabazas cada 128, escenario/baranda espejados en el centro (x 376).
   Leída como **una bahía de un salón largo**: al costado otra bahía con sus mismos tiles. Su paleta sale de la
   entrada del mapa (copiar la entrada entera, no el número de tile).
10. **Lo que es de la pelea no se repite**: la reja de dientes del centro y los tres contadores (su brillo se apaga
    con cada dado roto, `BosMapanime`) solo en la bahía del combate; en las otras, la cornisa y el panel del hueco
    entre dos contadores. Las rejas son sprites del jefe (`BosBoogieSaku`): no se tocan.
11. **Campo con figuras**: repetir 4 tiles en espejo da caleidoscopio; bloques de 8 x 7 tiles (las columnas verdes
    exteriores, 29 pegada a 0 como en el mapa), uno sí uno no en espejo y cada banda corrida 4 tiles, se lee como
    un estampado. Debajo del camino: sus líneas siguen su pendiente medida (filas 408..470) y los dientes negros del
    rojo van pegados a su línea; el antebrazo del esqueleto sigue hacia abajo con su última fila.
12. **Jafar** (24/10, aprobado por el usuario: "se veía genial", encendido para todos, `KHCOM_JAFAR_WIDE=0` lo
    apaga): no tenía vista ancha porque el bloque de sus pilares (y el del genio) es un búfer de RAM que en PC es un
    global nativo. La sala (48 x 48 tiles, x 448..831 envolviendo) se arma con períodos cortos: pared 2 tiles,
    columnas cada 31 alternando su espejo, orilla 7, lava 7 x 6 que cada 6 filas se corre 2 tiles. Fuera de la sala
    nunca se lee de las columnas de los pilares móviles (son de la pelea). El genio, uno solo (la copia más cercana) y
    sin las filas que el juego vacía al hundirse (entrada 0 = tile opaco, en la GBA quedan bajo la pantalla).
