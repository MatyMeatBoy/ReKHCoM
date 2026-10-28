# Fondo del nivel tras los jefes — 28/10/2026

## Causa y alcance

El caso de Jaula parásita se reprodujo con el ejecutable EU32 exacto de v2.2
(`04cfedd1…`), ROM propia previamente aislada y el cierre/reward real del juego.
BG0 seguía enlazado y válido: no era el rebind/origen mutable corregido antes
para Jafar. `Level_Up_0` marcaba **BG0 y BG1** como páginas de interfaz partidas,
aunque la escena de Jaula vive en BG0 y las estadísticas se cargan en BG1.
El compositor retiraba BG0 del mundo y dejaba transparente el centro partido.

Ahora sólo se divide la capa que recibe las estadísticas: BG0 en Úrsula y BG1
en los demás casos, igual que las cargas existentes de la propia función.
La limpieza al cerrar conserva ambos resets. No cambia PPU, cámara, zoom,
paletas, arte, progresión ni el código GBA sin `PLATFORM_PC`.
El cambio se apoya en #434 (`0f78f298…`), que corrige aparte el cuerpo de Jaula
al moverse a la izquierda. No modifica ningún paquete ya publicado de v2.2.

## Evidencia acotada

- Prueba CPU que compila los bloques PC **reales** de apertura/cierre:
  RED antes del cambio por asignar BG0 al menú; GREEN con 182 IDs de batalla
  × cuatro papeles de BG, incluyendo resets al cerrar. No es una prueba visual.
- Relink incremental EU32 desde 357 objetos de la build estable, leídos sin
  modificarlos. El baseline relinkado reproduce cuatro capturas byte por byte
  del ejecutable estable. Sólo se recompila la unidad cambiada; tras #434 se
  recompila también `bos_pc.c`. Identidades y comandos quedan en la evidencia.
- Jaula y Jafar: en el probe privado que encola un nivel y usa `bosskill` por
  la ruta real de daño/derrota, el hueco tiene 41/40 columnas negras antes y
  cero después. Los recortes opacos de opción/estadísticas permanecen idénticos.
  El comando privado no entra en `src/main.c` ni representa EXP natural completa.
- Original: nueve familias de arena × cinco cuadros de entrada/nivel/salida,
  45 comparaciones completas byte por byte idénticas antes/después. Tres
  inyecciones tardías quedaron en combate por un efecto activo: allí la identidad
  certifica ese estado de combate, no una pantalla de nivel que no se alcanzó.
- Inventario visual: las 29 entradas boss del debug (177–205), incluidos jefes
  humanos, tienen capturas de la secuencia; nueve arenas tienen además una pasada
  con fuentes HD realmente cargadas y zoom2, y Jaula/Úrsula/Jafar una pasada Riku.
  Fuentes HD no equivalen a una importación completa de medios HD externos.
- Combinada sobre #434: Jaula camina a la izquierda (600–1350), recibe daño
  mediante `bosskill` y completa su derrota; los comandos existentes
  `drawprizes`/`takeprize` recogen los premios por sus rutas habituales. El nivel
  aparece con cero enemigos/premios y los callbacks BG de mundo en `0000`:
  el cuerpo BG1 no invade las estadísticas. Normal/fuentes HD, dos pasadas
  completas: fondo continuo y cero columnas negras centrales al final.

## Límites de la auditoría de otros jefes

El `levelup` existente congela actores/efectos vivos para reutilizar sus tiles.
Si se inyecta mientras un efecto BG está activo, puede impedir el cierre:
Hook/Úrsula se capturaron antes de ese efecto; Darkside en un intervalo inactivo.
Trickmaster y la lámina de estadísticas de Úrsula muestran límites de ese fixture
tanto antes como después. No se presentan como regresiones causadas por este
cambio ni como bugs naturales confirmados.

La pasada privada de derrota de las otras siete arenas conserva registros y
capturas, pero no certifica el menú de nivel de todas: no sustituir esta matriz
por «todos los jefes aprobados». No hay ejecución nativa64, móvil o consola
aprobada por esta entrega. El centro GBA, los controles CPU y el caso reproducido
EU32 son evidencias distintas de esas plataformas pendientes.

Herramientas: `test_levelup_backdrop.py` (requiere `--cc`) y
`test_levelup_backdrop_render.py` (`--exe`, `--rom` privada, `--out` nuevo).
La opción `--natural` requiere el probe privado `rewardready`, no el ejecutable
normal. Evidencia privada conservada en `pruebas/boss-levelup-backdrop/`, fuera
del repositorio; no se distribuyen ROM, partidas ni capturas de usuario.
