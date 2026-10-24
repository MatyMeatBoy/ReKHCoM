# Empezar aquí (para quien retome el trabajo)

Este repo alcanza para seguir el proyecto desde cero en otra máquina: clonar, poner **tus propias ROMs** y seguir
la cola. Nada de este archivo pide archivos del PC del autor. Idioma de trabajo con el usuario: **español**.

## 1. Qué leer, en este orden

1. `port/progress/PENDIENTES_VIGENTES.md`: la cola única. El bloque **TRASPASO** más reciente (arriba) dice el estado
   y qué sigue.
2. `port/progress/REGLAS_DEL_USUARIO.md`: cómo quiere trabajar el usuario (publicar, aprobar arte, probar).
3. `port/progress/DEBUG_WORKFLOW.md`: clases de bugs ya vistas, cómo buscar sus parientes y las herramientas.
4. Del tema que toques: `HD_MOD.md` (mod HD), `BATTLE_SCENERY_TECHNIQUE.md` + `LOGICA_APROBADOS.md` (arte de vista
   ancha), `SWITCH_MOTOR.md`, `ONLINE_ESTUDIO.md`, `RENDIMIENTO.md`, `LINUX.md`, `ANDROID_64.md`.

## 2. Lo que no está en git (y cómo conseguirlo)

| Qué | Dónde va | Cómo |
|---|---|---|
| ROMs (EU `B8CP`, US `B8CE`, JP `B8CJ`) | `roms/B8CP.gba` etc. dentro del repo (ignoradas) | tu volcado del cartucho; SHA-1 en README.md |
| Toolchain Windows | carpeta `toolchain/` **al lado** del repo (`../toolchain`) | llvm-mingw (`llvm-mingw-*-ucrt-x86_64`), SDL3 3.4.18 mingw devel (`SDL3-*`), ninja en `toolchain/bin` |
| Python 3.12 | sistema | `pip install pillow numpy` (algunas pruebas: `uv run --with numpy --with soundfile`) |
| Android | `../toolchain/android` | SDK/NDK y SDL3 fuente: `port/tools/build_android.py --help` dice qué falta |
| Web | cualquier carpeta | emsdk; `build_all.py --emsdk ... --web-sdl-build ... --web-work ...` |
| Consolas (PS5, Vita, Switch) | una VM Debian | **texto, no imágenes de disco**: `port/progress/TOOLKIT_CONSOLAS.md` (qué instalar; devkitA64 de la imagen Docker `devkitpro/devkita64`) |
| Re:CoM (mod HD) | tu instalación de KINGDOM HEARTS HD 1.5+2.5 | el launcher importa: `ReKHCoM.exe --hd-import <juego> --scenes --quality 1` |
| ffmpeg (VMAF) | PATH | solo para `hd_quality_probe.py` |

Las rutas `D:/01Claude/ReKHCoM-build/...` de los .md son la **carpeta de trabajo pesada** del autor (builds de
consolas, emulador, pruebas grandes): en otra máquina usa cualquier carpeta con espacio. `KHCoMPC/jugar` y
`KHCoMPC/pruebas` son copias locales del usuario para probar: tampoco están en git.

## 3. Compilar (Windows)

```
python port/configure_pc.py --version eu --out build/pc-eu      # también us / jp
ninja -f build/pc-eu/build.ninja                               # build/pc-eu/khcom.exe y ReKHCoM.exe (launcher)
python port/tools/build_all.py --platform windows --package --release-version v2.0   # paquete (solo con OK)
```

## 4. Probar (todo oculto y en silencio: `SDL_VIDEO_DRIVER=dummy`, `SDL_AUDIO_DRIVER=dummy`)

| Herramienta (`port/tools/`) | Para qué |
|---|---|
| `check_sources.py` | compila port/src contra el shim de consolas (antes de cada commit) |
| `run_batteries.py --detach [--resume]` | baterías completas (escenas, eventos, salas, combates, Riku, mono, Android) |
| `debug_kit.py run/report` | accesos sueltos (`bios:`) que el PC recupera y Android/consolas no |
| `test_battles.py [--stock] [--riku]` | los 209 combates del menú de depuración |
| `arena_before_shots.py [--v2] [--only jefe]` | antes/después de las arenas de jefes a x3, 16:9 y 32:9 |
| `test_update.py --new <paquete>` | actualizar desde versiones viejas (zips en releases de GitHub) |
| `test_hd_scenes.py`, `test_hd_menus.py`, `test_hd_audio*.py` | mod HD |
| `test_android_scenarios.py` | Android en el emulador (APK `--smoke`) |
| `compare_ppu_builds.py`, `KHCOM_PPU_VERIFY=1` | la PPU no cambia un píxel |
| `hd_quality_probe.py` | peso y VMAF de la compresión de videos |

Variables del juego para pruebas: `KHCOM_BATTLE=<n>:<stage>:<riku>`, `KHCOM_KEYS=60-63:A,...`, `KHCOM_FAST=1`,
`KHCOM_QUIT=<cuadro>`, `KHCOM_OUTPUT_SHOT=<cuadros>`, `KHCOM_WINDOW=WxH`, `KHCOM_TIMING=1`, `KHCOM_PPULOG=1`.

## 5. Publicar

Solo con el OK del usuario. Lista de verificación en PENDIENTES_VIGENTES.md ("Lista de verificación antes de
publicar"). **Nunca borrar ni reemplazar un release ni un archivo publicado** (Quiver Launcher lo exige).
