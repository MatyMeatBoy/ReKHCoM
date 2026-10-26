# Herramientas de producción de tráiler

## Producción v2.0 (26/10/2026)

El flujo nuevo está separado en `v20_capture.py`, `v20_music.py`,
`v20_edit.py` y `verify_v20.py`. Conserva las entregas anteriores.
Requiere Python con Pillow/NumPy, ffmpeg/ffprobe y material privado propio.

```powershell
$env:KHCOM_TRAILER_BUILD='D:/ruta/build-publica/eu'
$env:KHCOM_TRAILER_HD='D:/ruta/importacion-hd-actual'
$env:KHCOM_TRAILER_ROM='D:/ruta/ROM-europea-propia.gba'
$env:KHCOM_TRAILER_DIR='D:/ruta/intermedios-trailer-v20'
$env:KHCOM_TRAILER_DELIVERY='D:/ruta/entrega-trailer-v20'
py -3.12 port/tools/trailer/v20_capture.py --workers 2
py -3.12 port/tools/trailer/v20_music.py
py -3.12 port/tools/trailer/v20_edit.py
py -3.12 port/tools/trailer/v20_edit.py --assemble
py -3.12 port/tools/trailer/verify_v20.py
```

El arnés copia el ejecutable estable sin recompilarlo. Los procesos usan
vídeo dummy, renderer software explícito, audio dummy, ventana oculta,
prioridad de pruebas de fondo y dos hilos PPU. No toman el mando físico ni
modifican partidas/configuraciones instaladas. El editor usa una captura
real del launcher con paneo editorial: no simula interacción.

La captura privada del editor debe estar en
`$KHCOM_TRAILER_DELIVERY/launcher-tall/decompui_6.png`; el inventario exacto
de Atlas en `atlas_honors_inventory.json`. La toma `cinematic_hd.mov` es
un fragmento importado de `hd/movies/ROP.mp4` (7–19 s), preparado con ffmpeg
a 1080p60 junto a su audio; no se presenta como gameplay grabado.

Los sliders recortan B al mismo tiempo global en que comienza el barrido;
no reinician la acción de B. Las tomas de eventos comienzan cuatro segundos
después del inicio de la grabación para excluir el menú de depuración.
La música compara la misma frase desde su comienzo, con RMS 0,1 en ambos
arreglos; `v20_music.py` evita el silencio del arranque original.

Las tarjetas son inglesas, Palatino con línea dorada. Separan Windows,
Linux y Steam Deck estables de los paquetes beta; Switch se identifica
como **beta anterior v2.0-pre1**, no como paquete de la estable v2.0.
Todo el gameplay está grabado en Windows: las tarjetas no fingen capturas
de consolas. No se publica ningún vídeo desde estos scripts.

`verify_v20.py` decodifica el MP4 completo, comprueba 1080p60/audio estéreo
48 kHz, mide la diferencia de duración A/V y el pico de audio, exporta
contactos/miniatura y escribe descripción con capítulos e inventario.
Revisar visualmente esos contactos antes de entregar. El inventario de
honores distingue el patio de Guard Armor, Agrabah 109 y la versión exacta
de la escena 074; no atribuye honores al contexto de la arena de Darkside.

---

## Flujo histórico: tráiler v4 de v1.8.2

Workflow opcional de vídeo; requiere Windows, Python/Pillow, ffmpeg y ROMs
privadas en `roms/B8CP.gba`, `B8CE.gba`, `B8CJ.gba`. No se incluyen ROMs,
vídeos del juego ni fuentes de Windows en estos scripts.

Ejecutar desde este directorio, con builds release EU/US/JP terminadas.
`KHCOM_TRAILER_DIR` cambia la salida (default: ../arte/Trailer/v182 fuera del repo).
`KHCOM_TRAILER_SAVE` puede apuntar a un .sav para copiar a los arneses;
ninguna prueba escribe en el archivo original. Los mandos físicos se
inhabilitan solo en los procesos de grabación.

Orden:

1. `python trailer_rec.py` (clips y efectos en una pasada; x4 explícito).
2. `python music.py` (mezclador de la misma compilación).
3. `python intro_rec.py` (paseo de castillo largo y sin congelados).
4. `python intro.py`, `python sweep.py`, `python cards.py`.
5. `python launcher.py`, `python postcred.py` (el segundo puede omitirse si
   ya existe el clip de arte aprobado), `python composites.py`.
6. `python acts.py A B D E F`, `python final.py`.
7. `python verify_final.py` y revisión visual de sus hojas de contacto.

La intro usa un recorte de 240x160 píxeles nativos del paseo x4: ambas
mitades del slider comparten cámara y tiempo. Crece, revela 16:9 y ajusta
suavemente a x4. Música al empezar el slider (3,8s); poscréditos en silencio.
El montaje de mundos continúa la fase de ascenso por las escaleras.

La salida se revisa por hojas de contacto y decodificación completa. No
subir material sin revisar su duración, los textos y el final en silencio.
