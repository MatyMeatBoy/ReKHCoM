# Workflow: completar el arte de la vista amplia (20/10/2026)

Pedido del usuario: un solo procedimiento para generar el arte que falta fuera de
los 240x160 originales (salas, escenas, escenarios de combate y sus eventos),
**duplicando tiles del juego y dibujando pixel art solo donde sea necesario**.
Reúne lo aprendido en LOGICA_APROBADOS.md (reglas), BATTLE_SCENERY_TECHNIQUE.md
(escenarios de combate) y PRUEBAS_VISUALES.md (cómo entregar). Ante la duda,
mandan las reglas de LOGICA_APROBADOS.md.

## 1. Encontrar todas las apariciones del fondo

Un mismo fondo sale en más de un sitio: el combate, el evento que lo precede,
la derrota, la revancha. Guard Armor (20/10) tenía el patio aprobado en el
combate, pero la escena 008 (las piezas caen y se arman) usaba el mismo
fondo sin la extensión, porque esta exigía el modo combate.

- Identificar el fondo por sus datos (los mapas o tiles exactos), nunca por el
  modo, el número de combate o una bandera de estado.
- Buscar dónde se carga ese fondo (`grep` del símbolo del mapa en `src/`) y
  probar cada sitio: combate (`KHCOM_BATTLE`), evento (`KHCOM_EVENT=lista:índice`,
  ver `port/tools/test_events.py`), sala (`KHCOM_MODE=...:room...`).

## 2. Leer el arte original

1. Decodificar mapas, tiles y paletas (`KHCOM_ROOMDUMP=1` + `room_ascii.py` en
   salas; en combates, los mapas del cartucho, como `port_guard_armor.c`).
2. Ubicar el rectángulo realmente ilustrado: un mapa de 512x512 puede tener
   zonas vacías; repetir el mapa entero repite también sus huecos.
3. Clasificar cada zona:
   - **Única** (torre, escalera, marco de ventana, relieve, figura, jefe):
     nunca se copia ni se corta.
   - **Banda repetible** (fachada, tejado, arcos, pared lisa, cielo, pavimento):
     se continúa con su propio período y dirección.
   - **Borde** (festón, cornisa, bordillo): cierra el mundo donde el original
     termina en corte recto.
   - **Decoración ambiental repetible** (puestos, grupos de jarrones): piezas
     completas del propio escenario que pueden poblar su continuación cuando
     encajan con el ambiente; distinguirlas de los adornos únicos.

## 3. Generar el arte, en este orden de preferencia

1. **Duplicar tiles del juego**: repetir la banda con su período, voltear un
   tile (bits 0x400 / 0x800 de la entrada) para cerrar un borde, usar la pared
   lisa (el tile de BG3 más repetido de la columna, `MapPlainBaseTile`).
2. **Muestrear píxeles del juego** donde el tile no encaja (suelos en
   perspectiva): fila por fila, con los colores y límites de las celdas
   originales, como el pavimento de Guard Armor.
3. **Dibujar pixel art** solo si no existe tile ni píxel adecuado (autorizado
   por el usuario el 18/10):
   - mismos índices de paleta del fondo (nada de colores nuevos), mismo grosor
     de línea, mismo sombreado y misma rejilla de 8x8;
   - el dibujo continúa el del original (perspectiva, período, luz);
   - va en el repo con una nota de qué tile imita y por qué no bastaba copiar.

### Mejoras aprendidas en Agrabah 107/108/109 (aprobadas con honores, 21/10)

- Cerrar primero la estructura: una tienda no sustituye la reparación de un
  borde cortado. Seguir cada diagonal por separado y conservar todas las filas
  de sus tiles, incluidas las dos paridades de una línea 2:1.
- Comprobar que el donante contiene dibujo en cada columna. Si lo corta el
  límite del mapa, avanzar hacia otra tira intacta por la misma diagonal;
  no reemplazar parte del bordillo por pared lisa.
- Para dar continuidad al ambiente, reutilizar piezas decorativas completas
  del propio juego. Intercalar tipos y grupos, variar las separaciones y dejar
  descansos; mantener anclaje al suelo, perspectiva y escala. No repetir una
  cuadrícula de puestos ni clonar marcos, relieves o figuras únicos. Las
  posiciones concretas se deducen del fondo nuevo, no se copian de Agrabah.
- La decoración adicional es arte de fondo: no añadir objetos de lógica,
  colisiones, personajes o entradas OAM. Respetar la paleta activa de la fuente,
  también cuando la pieza original usa paleta OBJ.
- Comparar la misma instantánea y ejecutable antes/después: el avance del
  texto y la animación pueden diferir entre arranques independientes.
- En cada nueva pieza de la tanda, apagar solo su receta para los controles,
  manteniendo activadas las anteriores. Si una tira limpia mide un tile y la
  diagonal necesita alternar 4 px, usar muestreo de píxeles con ese desfase;
  no recortar el borde ni aceptar fragmentos del adorno vecino como donantes.
- Revisar cada capa exportada por separado además del compuesto: una celda
  transparente puede parecer cubierta por otra capa en el inventario y quedar
  negra en los márgenes ampliados. Rellenar solo los píxeles sin capa visible
  con la superficie que corresponde a su zona (pared arriba, suelo abajo),
  manteniendo únicos los objetos de la capa decorativa.

En todos los casos:
- Guardar **índices de paleta** y resolverlos con la paleta activa en cada
  frame: los fundidos y efectos de color del juego siguen funcionando.
- Dibujar solo fuera de los 240x160; el rectángulo GBA queda idéntico.
- No ampliar colisiones, ni cámara, ni duplicar personajes o enemigos.
- Todo bajo `PLATFORM_PC`, con una variable para apagarlo en las pruebas de
  antes/después (p. ej. `KHCOM_NO_GUARD_PATIO=1`, `KHCOM_TOPCLOSE=0`).

## 4. Probar (el usuario lo pide así)

- **Siempre en x3 entero**: `zoom=6` en `kh_com.ini` (escala 3, píxel entero).
  Una escala grande esconde los fallos de las zonas lejanas.
- Ventanas cada vez más anchas, todas en x3:
  `KHCOM_WINDOW=1920x1080`, `2560x1080`, `3440x1440` (más alto: se ve más
  cielo y suelo) y `3840x1080` (32:9).
- Varios fotogramas de cada fase: entrada, animación propia (caídas, saltos,
  ataques que mueven la cámara), fundidos y salida.
- Mismo ejecutable con la extensión activada y apagada: rectángulo GBA
  idéntico; los demás fondos, idénticos en toda la imagen.
- Revisar a ojo en las capturas anchas: costuras, escalones en diagonales,
  bandas que no casan, adornos cortados o repetidos.
- Compilar y ejecutar las comprobaciones en serie, una tarea de juego a la vez,
  para cuidar RAM/CPU mientras el usuario juega.

## 5. Entregar y cerrar

Pedido actualizado del usuario (21/10): preparar **Preliminar V1** con todas
las escenas que necesitan extensión y todas las peleas de jefes pendientes.
No parar a pedir aprobación de cada pieza. Aplicar el workflow mejorado a toda
la tanda; conservar las versiones ya aprobadas y sus recetas.

**Precisión en la tanda (indicación del usuario, 21/10): terminar una pieza
antes de avanzar a la siguiente.** El inventario puede prepararse en conjunto,
pero cada escena/jefe debe recorrer completo el ciclo de estudiar el original,
elegir donantes, implementar, revisar capturas, corregir, comprobar y cerrar su
entrega. Solo entonces empezar la siguiente. Una compilación general o unas
capturas iniciales no cuentan como piezas terminadas. Mantener una lista
explícita de terminadas y pendientes; no trasladar errores a toda la tanda.

1. Implementaciones reales, listas para integrar: código del renderizador,
   paletas activas, pruebas y documentación. No entregar montajes ni conceptos
   como sustituto de los fondos funcionando en el juego.
2. Copia jugable completa + lanzadores, índice de escenas/jefes y capturas x3
   antes/después en los cuatro anchos. Registrar por pieza qué se comprobó,
   qué cambia y sus límites; los vacíos intencionados no se rellenan por sistema.
3. El usuario comenta el conjunto Preliminar V1. No marcar cada propuesta como
   aprobada ni publicarla por el mero hecho de formar parte de la tanda.
4. Congelar recetas y mejoras en commits revisables. **Codex hace los commits;
   Claude integra**. Registrar las aprobaciones reales en LOGICA_APROBADOS.md.
