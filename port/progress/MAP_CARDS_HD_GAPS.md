# Naipes mapa HD: contador, ilustraciones e importaciones anteriores

Bloque de la prioridad2.3 del usuario, sobre main `e2324ba5`. **Parcial:**
se cubren25 de26 familias pequeñas. `gKeyToRewards` conserva su ilustración
original: no se identificó una contraparte en las texturas Re:CoM examinadas.
No sustituirla por otra carta ni presentar un escalado de píxeles como arte HD.
Todavía no forma parte de la estable2.2.5 ni de los tests anteriores inmutables.

## Cambios y mecanismo

- El total generaba dos nueves en tiles1f/20, pero las dos variantes del mapa
  muestran los nueves fijos9e/9f. Se registra el significado HD de esos tiles
  usando sus píxeles originales y la paleta del contador generado. La barra
  también recibe registro explícito: un tag antiguo podía bloquear su detección.
  No se cambia ninguna copia DMA ni el dibujo del modo original.
- El generador sólo recorría el catálogo de combate. Ahora incorpora las26
  familias de `gMapCardDefs`, añade25 recetas pequeñas y tres ilustraciones
  grandes de llaves de historia. Mantiene las853 recetas existentes intactas.
  La primera llave adopta el símbolo de llave de Re:CoM, distinto del signo
  de interrogación GBA; sus reglas siguen siendo las del original.
- Las nuevas imágenes sólo cubren la ventana14×15 del naipe pequeño. Corona,
  bordes y dos esquinas siguen siendo originales. Las llaves de historia no
  reciben el recorte reservado al valor de las cartas numeradas. `--only --write`
  conserva las recetas aprobadas que no se estén regenerando.
- El launcher sólo comparaba PNG ya presentes y omitía los recursos nuevos
  ausentes. El inventario v2 registra las recetas intentadas, también cuando
  una edición no contiene su textura. Una receta nueva/cambiada provoca
  actualización; una ausencia ya conocida conserva fallback sin bucle.
  Los inventarios antiguos necesitan una actualización de gráficos. El productor
  Python completo escribe el mismo formato; `--only` no certifica el conjunto.

En Windows, la detección existente actualiza sólo gráficos si encuentra la
instalación fuente. Linux conserva su acción explícita `Import owned PC sprites`
(`--import-sprites`); no se añade ni se certifica actualización automática allí.
Android recibe la carpeta HD preparada. Los datos fuente e imágenes importadas
pertenecen al jugador y no se incluyen en el repositorio ni en paquetes.

## Verificación

- Emisores/registro reales en prueba CPU:100 totales0–99, firmas de los nueves,
  barra con tag previo, cuatro DMA originales y vecinos sin cambios. Falla antes
  de las correcciones y pasa después.
- Launcher real: migración heredada, receta ausente nueva/cambiada, ausencia
  conocida y carpeta vacía. La omisión de recetas nuevas falló en el ejecutable
  anterior. También se compila/ejercita el escritor C++ real y se verifica el
  productor Python, sin entradas propietarias.
- Importación C++ real, sólo gráficos desde instalación propia:881 archivos,
  los28 outputs nuevos coinciden en tamaño/transparencia con Python, con una
  diferencia máxima de1/255 por canal en el redondeo del filtro. `--hd-stale`
  devuelve0 después, sin recargar vídeos/música. Un recorrido adicional del
  consumidor Windows64/Direct3D11 usa directamente esos outputs C++ y pasa.
- Diez recorridos del juego con copia de una partida USA: Windows32/64 antes y
  después, HD off/on; cuadrícula, scroll, categoría dorada, detalle y reentrada.
  Se añaden assets ausentes y64 con Direct3D11 confirmado por el renderer.
  Todos terminan con código0 y cinco capturas. Las diez comparaciones HD off
  son idénticas;48 regiones de las esquinas nativas también son idénticas.
- Lectura de ROMs propias:78 cajas OAM (26 familias×3 regiones), arte/paletas
  pequeños idénticos entre regiones; siete fuentes numéricas coinciden tras
  aplicar los offsets DMA de localización. Rama original sin `PLATFORM_PC`
  preprocesada sin cambios. `check_sources.py`, tablas generadas y ABI64 pasan.

Evidencia privada en `pruebas/mapcards23/`: `RUNTIME.json`, `CONTRACTS.json`,
`IMPORT-VERIFIED.json`, `NATIVE-IMPORT.json`, `NATIVE-CONSUMER.json`, logs y
capturas. No mide continuidad de audio, ni
certifica juego físico Android/Linux/consolas o las25 ilustraciones en todas
sus transiciones. Siguen pendientes la puerta de recompensas y su comprobación.
