# Pósteres HD pequeños y carga fuera del dibujo

La demora medida viene de abrir Media Foundation por cada selección, aunque solo se pide un frame. Probe de la
función real con `ROP.mp4` importado, prestado en solo lectura: 422 ms frío, 219–234 ms repetido, pico 194376 KiB.
La reducción anterior a 160×90 también perdía detalle antes de ampliar el cartel de la galería.

Se eligió PNG de 640×360 (360p, bajo el máximo de 480p solicitado), un frame por segmento. La ruta precalculada
se lee antes de iniciar el decoder. El probe aislado del lector real de un PNG da 0–16 ms y aproximadamente
12 MiB; no representa el consumo total del juego. Archivo de ejemplo real: 276688 bytes. No se guarda video ni
una secuencia de frames como vista previa.

- `PortHdScenePreviewPath` resuelve y copia la ruta en el hilo principal. `PortHdScenePreviewFile` trabaja solo con
  esa copia, sin renderer ni `PortConfigPath` compartido. Windows inicializa/libera COM en el hilo que lo llama;
  Linux acepta las dimensiones acotadas y pide un solo frame a ffmpeg.
- Sidecar importado: `<movie>.poster-v1-t<second>-640x360.png`. Un PNG viejo respecto al MP4, demasiado grande,
  corrupto, de otra resolución u otro segmento no se reutiliza. Offset normal 1 s; colas compartidas 171/223 s.
- El importador Windows genera los pósteres después de cada película y completa los faltantes de películas que
  conserva. Es opcional: un codec o una escritura fallida no rompe ni reemplaza la película. Se escriben primero
  archivos temporales y se registran los nuevos en el journal de importación.
- La referencia Python hace lo mismo con ffmpeg. `khhd_posters.py <hd>` permite preparar un import existente sin
  recodificar sus películas. No se añade ffmpeg como dependencia del importador Windows.

La galería y su textura a resolución de salida pertenecen al cambio coordinado del otro carril: un único job
asíncrono y un póster residente; no convertir otra vez la imagen a un canvas de 90 píxeles antes de mostrarla HD.
Los decoders de esta API son Windows/Linux; los pósteres PNG ya importados son archivos portables para las otras
plataformas. No se afirma con estos probes que exista extracción nativa HD en cada backend.

Validación: cuatro pruebas offline de importación (un frame/offset, caché vigente, error/timeout sin perder el
póster anterior y timestamp futuro), probe C del lector real (offset incorrecto, viejo, bounds y COM propio) y
probe C++ del helper nativo sobre película real. Este último detectó en rojo una caché corrupta que se daba por
vigente; en verde la regenera, mantiene cancelación y preserva SHA256 de la película fuente. Los ejecutables,
DLL y enlace temporal de película de los probes se eliminan automáticamente. Evidencia pequeña local:
`D:/01Claude/ReKHCoM-build/poster-perf-small/` (JSON, código del probe y un PNG).

Revisión: lectura UTF-8 por SDL IO y atributos Windows con UTF-16. Se reprodujo rojo y verde tanto un PNG como
un decoder real a través de `画面-Ñ.mp4`, sin copiar el vídeo. Los callers de la API File/Path deben seguir
condicionados a Windows/Linux; el formato PNG se puede leer de manera portable sin ese decoder.
El fallback WMF es síncrono dentro del job: ocho samples limitan cantidad, no garantizan un deadline del codec.
Cerrar la galería no debe esperar ese job; el cierre final coordina sus recursos antes de apagar SDL.
