# Texto ausente en la pausa de películas HD — Pedro, issue #163

Reporte del 26/10/2026: https://github.com/MatyMeatBoy/ReKHCoM/issues/163,
video https://www.youtube.com/watch?v=qkZSuuTsrio. El modal conserva panel, líneas y
cursor, pero no muestra título ni opciones. La captura facilitada y el video
coinciden con la reproducción local. El texto de la galería sí aparece.

## Causa y arreglo

Con `hd_title=1`, `PortUiDraw` dibuja el panel en la imagen del juego y difiere las
letras a una cola HD. El presentador normal ejecuta luego `PortUiHdDraw`. El bucle
propio de pausa de películas (`MoviePause`) solo presentaba la imagen del panel;
nunca ejecutaba ese segundo paso. No faltan fuentes ni traducciones en ese caso.

La prueba con una ROM JP real encontró además un pariente: la cola UI guardaba
Unicode, pero `GlyphJp` de GDI/FreeType/web espera Shift-JIS. El camino software
ya hacía la conversión. Ahora el productor de la cola JP usa esa misma conversión;
esto corrige también los demás paneles UI japoneses que utilizan la cola HD.

Ahora compone las letras después del panel, en la resolución vertical de salida,
y hace coincidir su transformación horizontal con el panel incluso si se cambia
la proporción de la ventana. Restaura la escala del renderer al terminar. La
captura del arnés ahora lee la imagen realmente presentada, incluidas las letras,
y guarda tanto Pausa como la confirmación Sí/No.

## Alcance y parientes

- Windows y Linux de 32 bits comparten `MoviePause`: el arreglo cubre ambas rutas.
- Las tres regiones EU/US/JP comparten esa composición. Se compila el archivo real
  para Windows x86 con los tres defines; la regresión también ejecuta la composición
  con los defines de Windows/Linux y las tres regiones.
- Android 32/64 y Linux64 utilizan reproductores propios: saltan directamente sin
  crear este modal. No se introduce un modal nuevo ni se afirma una prueba física.
- Auditoría de todos los calls a `PortUiDraw`: solo hay dos. El presentador normal
  ya compone `PortUiHdDraw`; el único que lo omitía era `MoviePause`. Sus dos estados
  (pausa y confirmar salto) tienen el mismo arreglo. `hd_title=0` conserva la fuente
  GBA y se verifica en la misma prueba.
- Las miniaturas negras tras volver repetidamente a la galería se investigan en
  un carril independiente; esta entrega no las declara corregidas.

## Evidencia

- RED `test_hd_movie_pause_layers.py`: `HD pause deferred text never presented`.
  GREEN: cinco escenarios de composición por plataforma/región (30 escenarios):
  HD/GBA, ventana normal, ancha, cuadrada y escalas fraccionarias; orden de capas,
  texto presentado y escala restaurada.
- Reproducción Windows con SDL vídeo/audio `dummy`, archivo real `zz_event_151`:
  antes, modal sin letras; después, título y ambas opciones legibles.
- `test_hd_movie_pause_text.py`: seis sesiones aisladas, cinco idiomas EU
  (en/es/fr/de/it), HD/GBA y ventana redimensionada. Las doce capturas de ambos
  modales pasan los controles de píxeles del título y de cada fila de texto.
  Root revisó visualmente Pausa en inglés y confirmación en francés; también se
  inspeccionó la confirmación alemana, cuyo panel es más ancho por su título.
- Las pruebas no usan partidas ni cambian los archivos importados. Capturas en
  `build/ending-sync/{before,after}-popup` y `build/popup-text-runtime2`, fuera de git.
- ROM JP real, build nativo JP completo: antes del segundo arreglo seguía sin
  letras; después ambos modales japoneses muestran kana/kanji. Se verifican fuente
  HD y GBA en dos sesiones (cuatro capturas), con su ROM B8CJ.
- Build nativo US completo con ROM B8CE: dos sesiones HD/GBA, cuatro capturas
  de ambos modales en inglés, todas pasan. En total: diez sesiones, veinte capturas.
- `test_hd_ui_japanese_queue.py`: RED guarda Unicode 4e00 en vez de CP932 88ea;
  GREEN con la conversión real de Windows y conservación de letras latinas.
- `check_sources.py`: OK. No nuevas baterías generales, release ni comentarios externos.
