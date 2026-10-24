# Tráiler v4 de v1.8.2

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
