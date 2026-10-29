# Naipes mapa HD: contador, ilustraciones e importaciones anteriores

Prioridad2.3 del usuario: contador99/barra y las26 familias pequeñas HD
implementados, con verificación Windows32/64 descrita abajo. PR492 cubrió25;
la ilustración de recompensas se encontró después en el atlas del menú de
Re:CoM, `FORM/0050/FO0050.RTM:UK_card.imz#19`, celda384,128..512,256.
Se adapta desde esa fuente del jugador, tanto pequeña como grande, sin
redibujarla ni distribuirla. Todavía no forma parte de la estable2.2.5 ni
de los tests anteriores inmutables; queda para la próxima entrega.

## Cambios y mecanismo

- El total generaba dos nueves en tiles1f/20, pero las dos variantes del mapa
  muestran los nueves fijos9e/9f. Se registra el significado HD de esos tiles
  usando sus píxeles originales y la paleta del contador generado. La barra
  también recibe registro explícito: un tag antiguo podía bloquear su detección.
  No se cambia ninguna copia DMA ni el dibujo del modo original.
- El generador sólo recorría el catálogo de combate. Ahora incorpora las26
  familias de `gMapCardDefs`, añade26 recetas pequeñas, tres ilustraciones
  grandes de llaves de historia y la grande de recompensas. Mantiene las853
  recetas anteriores al primer bloque intactas; el complemento de recompensas
  también conserva intactas las881 filas de PR492.
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

## Complemento de recompensas y cobertura completa

Buscar sólo texturas independientes `CARD/06xx/07xx` dejó esta ilustración fuera.
El inventario de15475 nombres remasterizados y la inspección de las subimágenes
del menú localizaron la puerta con sus destellos en `UK_card.imz#19`. Su imagen
decodificada es idéntica en las cinco entradas de idioma; no contiene texto.
La [referencia pública de Re:CoM](https://www.khwiki.com/File:Key_to_Rewards_(card).png)
sirvió para identificarla; no se descargó ni se usó su archivo como recurso.

- Test de cobertura: antes falla por `gKeyToRewards`; después pasan cinco
  comprobaciones del generador/inventario, con26/26 recetas pequeñas.
- Actualización real del importador C++: copia privada del import881 anterior,
  detecta exactamente dos recetas nuevas; produce883 archivos y queda stale0.
  Los881 outputs anteriores conservan sus bytes. Ambos nuevos coinciden con
  Python: grande exacta, pequeña con máximo1/255 de redondeo; sin recargar media.
- Fixture privada creada por el editor real sobre una copia: una carta por
  familia válida,26 entradas, slots reservados vacíos y checksums correctos.
  El contador general muestra22 porque el original excluye las cuatro doradas;
  las categorías conservan9 rojas,7 verdes,6 azules y4 doradas.
- Ocho recorridos adicionales: listado en tres posiciones que cubren26 familias,
  detalle de recompensas y reentrada; Windows32, Windows64 software/Direct3D11,
  HD apagado y ausencia sólo de las dos imágenes nuevas. Cinco comparaciones
  HD off y cinco de fallback son idénticas al binario anterior. Todos exit0.
- Tres recorridos más toman cinco fases del brillo premium:32 antes/después y
  64Direct3D11. La carta pequeña HD es visible en las cinco muestras; ocho
  regiones de esquinas originales entre las cuatro doradas siguen idénticas.
  No se cambia la animación ni el timing original.

Evidencia del primer bloque en `pruebas/mapcards23/`: `RUNTIME.json`, `CONTRACTS.json`,
`IMPORT-VERIFIED.json`, `NATIVE-IMPORT.json`, `NATIVE-CONSUMER.json`, logs y
capturas. Complemento en `rewards/`: `PREPARED.json`, `NATIVE-UPGRADE.json`,
`RUNTIME.json`, `GOLD-CYCLE.json`, `CONTRACTS.json` y sus capturas. Este bloque
no mide continuidad de audio ni certifica juego físico Android/Linux/consolas
o cada estado posible de las26 cartas. Continúan diario y audio de prioridades2/3.
