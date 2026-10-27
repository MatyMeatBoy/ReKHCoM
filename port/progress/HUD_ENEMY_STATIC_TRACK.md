# Medidor enemigo HD: geometría fija y equivalencia pendiente

Reporte del usuario, 27/10/2026: la longitud máxima visible crece, vuelve al
inicio y repite aun sin cambiar la vida. Carril `hud-enemy-gauge-static-track`,
owner `codex-enemy-gauge`, rama `fix/hud-enemy-static-track`.

## Causa y cambio acotado

`PortHdEnemyGauge` incrementaba un reloj `anim`; `PortHdEnemyGaugeDraw` lo
enviaba al borde y a dos componentes del contador. `PortHdLayoutDraw` reproduce
los grupos en bucle y calcula sus transformaciones por cuadro. No era un cambio
de `actor->maxHp`: el tiempo modificaba la geometría con vida constante.

Se inspeccionó una importación local existente de `gHudGaugeLad.lad`, sin
modificarla ni incluir sus bytes. En la secuencia 1:

| Grupo | Función observada | Cuadro ahora |
|---|---|---|
| 0 | Borde/cuerpo escala X de 0 a 1; punta cambia pivote de 0 a −200, cuadros 0..200 | 200 fijo |
| 1 | Relleno rojo, escala X de 0 a 1 | Valor del rastro, fijo |
| 2 | Superposición constante, transformaciones fijas 0..200 | 0 fijo |
| 3 | Relleno verde, escala X de 0 a 1 | Valor de HP, fijo |
| 4, 5, 6 | Componentes acumulativos del contador, comienzos 1..15 | Capas restantes, fijo |

El decompilado local previo `hud_ranges.c`, función `FUN_140207c70`, inicializa
la longitud en 200 y fija los lugares de relleno y contador por valor. Esto
corrobora que los grupos son controles de datos. No constituye una comparación
visual completa contra una ejecución actual de Re:CoM 1.5.

El cambio conserva la escala normalizada de verde/rojo ya existente: una capa
de hasta 560 HP ocupa como máximo 200 cuadros. No modifica estadísticas,
daño, curación, selección de enemigo ni estados de combate. La equivalencia
exacta de longitud por capacidad con GBA/1.5 queda pendiente abajo.

## Prueba y límites

`python port/tools/test_hd_enemy_gauge.py` extrae y compila las funciones reales
de actualización y dibujo, y registra los controles enviados al layout. Antes
del cambio falla en el borde del primer cuadro; después verifica 1.000 cuadros
constantes, capas 0/560/1120/1680/2240 y vecinos, rastro tras golpe, demora y
cierre, curación, cambio de máximo, objetivo, invisibilidad y salto de cuadros.
También comprueba que un layout/textura no disponible conserva el fallback.

Además pasan `test_hd_layout_ready.py` y `test_hd_gauge_targets.py` (con `CC`
apuntando al clang de llvm-mingw existente). `check_sources.py` detecta en la
base `633d637` un uso de `SDL_EVENT_RENDER_TARGETS_RESET` en `port_main.c`
ausente del shim SDL de consola; esos archivos no se modifican en este carril.

No usa ROM, GPU, partidas ni instalación Quiver. Esta prueba demuestra controles
estables y evolución de HP/rastro, no suavizado ni equivalencia visual. Falta
reconstrucción serial y captura GPU con el paquete combinado del coordinador.

## Pendientes conservados

- **Jugador 155/155 y equivalencia de capacidad:** la evidencia local tiene
  HP actual y máximo 155. GBA elige tamaño 3 (121..160) y ratio 256 al estar
  lleno. El layout HD usa `floor(valor/560*470)`: verde y capacidad llegan al
  cuadro 130, por eso el arco puede verse corto sin daño. El fallback de imagen
  de anillo usa `displayHp/maxHp` y muestra el anillo completo con 155/155.
  Comparar ambos caminos y referencia 1.5, incluyendo HP 0/1, 155/155,
  límites 280/281/321/322/560 y máximos superiores; documentar saturación y
  monotonía antes de elegir la equivalencia. No cambiar reglas del juego.
- **Longitud máxima enemiga:** el arreglo estabiliza la longitud normalizada
  existente; GBA también cambia el tamaño visual por tramos de máximo HP.
  Comparar capacidades pequeñas y capas con referencias del juego y 1.5 antes
  de afirmar equivalencia exacta o cambiar la escala de verde/rojo.
- **Bordes internos dentados:** el supersampling reserva el rectángulo completo
  de 680×280 unidades, aunque sólo haya un arco corto. A escala de juego 9,
  SS2 requiere aproximadamente 43,4 MB, superior a 32 MiB: cae a geometría
  directa. SS4/SS2 cuestan 21/5 superficies RGBA base; equipos limitados tienen
  12 MiB. Software, `KHCOM_GAUGE_NO_SS` y fallos de asignación también omiten
  supersampling. Registrar backend/escala/ruta efectiva y capturas GPU
  720/1080/1440 antes de atribuir la captura a una causa concreta. Evaluar
  recorte al área visible o procesamiento por partes conservando presupuestos.

Estos pendientes no son promesas de porcentaje ni de paridad ya verificada.
