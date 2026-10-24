# Cómo preparar una prueba visual para el usuario (aprendido 18/10/2026)

El usuario revisa a ojo en su PC, así que cada cambio visual se entrega como
copia jugable aparte, nunca sobre sus carpetas ReKHCoM-vX (esas se conservan).

## 1. Reproducir y capturar (para uno mismo)

- Compilar: `python port/configure_pc.py --version eu --out build/pc-eu` y
  `ninja -f build/pc-eu/build.ninja` (toolchain en `../toolchain`).
- Ejecutar sin audio ni ventana: `SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy`.
- Variables útiles: `KHCOM_MODE="frame:modo:arg,..."` (p. ej. `roomriku`,
  `roomshape`, `roomseed`, `roomgoto` llevan a una sala exacta), `KHCOM_KEYS`
  (teclas por rango de fotogramas), `KHCOM_WINDOW=1920x1080`, `KHCOM_FAST=1`,
  `KHCOM_QUIT=<fotograma>`, `KHCOM_OUTPUT_SHOT=f1,f2` (guarda `out_<f>.bmp`).
- Convertir los BMP a PNG y recortar/ampliar la zona para revisarla.

- Arte de la vista amplia: probar siempre en x3 entero (`zoom=6`) a 1920x1080,
  2560x1080, 3440x1440 y 3840x1080 (WORKFLOW_ARTE_AMPLIO.md).

## 2. Entregar la copia al usuario

1. Carpeta nueva en `KHCoMPC/pruebas/` (21/10: las copias jugables de versiones van en `KHCoMPC/jugar/`): descomprimir el último paquete
   Windows (`build/releases/ReKHCoM-<ver>-windows-x86.zip`) en
   `ReKHCoM-<ver>-<tema>/`.
2. Sustituir `bin/khcom_eu.exe` por `build/pc-eu/khcom.exe`.
3. Copiar las ROM a `roms/` y `roms/B8CP.gba` como `bin/baserom.gba`
   (el exe suelto busca `baserom.gba` a su lado).
4. Crear junto a ella, en `pruebas/`, un `Probar <tema>.bat` que fije `KHCOM_MODE` (para ir
   directo a la sala) y lance `bin/khcom_eu.exe`. En .bat usar `/` en las rutas
   internas; no usar `\b`/`\r` en plantillas de texto.
5. Decir al usuario qué .bat abrir; esperar su aprobación antes de activar
   algo por defecto o publicar.

## 3. Referencia: borde superior de la sala de Riku (#17)

`src/map/map_cell.c`: `MapFindEdgeStrip` (tira inferior de 24x4 tiles),
`MapTopEdgeTile` (volteada arriba) y `MapPlainBaseTile` (pared lisa de BG3).
`KHCOM_TOPCLOSE=0` lo apaga. Ver LOGICA_APROBADOS.md y `port/tools/room_ascii.py`.
