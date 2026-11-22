# ReKHCoM para R36S

El paquete de prueba usa Linux AArch64 en el RK3326. Requiere un firmware de 64 bits con glibc 2.31 o superior, KMSDRM/EGL/GLES y ALSA. El host conserva el juego ARM64 con punteros de 32 bits para mantener sus estructuras de datos. El paquete contiene SDL3 y utiliza el controlador gráfico y las bibliotecas de audio del firmware.

## Instalación manual

1. Extrae el ZIP en la carpeta `ports` que usa EmulationStation. El script `ReKHCoM R36S.sh` queda junto a `rekhcomr36s/`.
2. Pon tu volcado de cartucho GBA europeo, estadounidense o japonés en `rekhcomr36s/roms/`. La región se detecta por la cabecera. Por terminal puedes seleccionar una con `--region eu`, `us` o `jp`.
3. Abre **ReKHCoM R36S** desde **Ports**. Usa **Start + Select** para abrir las opciones del juego y elige **Salir** para volver al frontend. Si PortMaster aporta un mapa SDL para el mando, el script lo carga antes de iniciar.

Las partidas, preferencias y `ReKHCoM.log` quedan en `rekhcomr36s/`. Para un fallo de arranque, conserva ese registro junto con el modelo, panel y versión de firmware. `SDL_VIDEO_DRIVER` y `SDL_AUDIO_DRIVER` se pueden especificar para diagnósticos. El ZIP está preparado para instalación manual. La validación del panel, mandos, audio y rendimiento en la R36S física queda pendiente antes de enviar el port al catálogo.

En una pantalla de 640×480, el perfil de juego usa un lienzo de 320×240 a 2× con filtro de vecino más próximo. La imagen de GBA ocupa 480×320 y el compositor amplía las escenas compatibles con los mismos fondos del modo 4:3. Los menús y las cinemáticas conservan su encuadre propio.

## Compilación

Prepara un sysroot AArch64 Ubuntu 20.04 Focal con GCC 10, los paquetes de desarrollo de DRM, GBM, EGL/GLES y ALSA, y los archivos de `pkg-config` asociados. Usa la misma versión de SDL3 que el resto del proyecto y limita `pkg-config` al sysroot para que CMake detecte las bibliotecas de destino. La configuración de SDL debe indicar que KMSDRM, EGL/GLES y ALSA están habilitados. Las bibliotecas de esos backends deben resolverse durante la ejecución en el firmware.

```sh
export REKHCOM_R36S_SYSROOT=/ruta/focal-arm64
export PKG_CONFIG_SYSROOT_DIR="$REKHCOM_R36S_SYSROOT"
export PKG_CONFIG_LIBDIR="$REKHCOM_R36S_SYSROOT/usr/lib/aarch64-linux-gnu/pkgconfig:$REKHCOM_R36S_SYSROOT/usr/share/pkgconfig"
cmake -S /ruta/SDL -B build/r36s/sdl -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/port/linux/r36s/aarch64-focal.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/ruta/SDL-focal \
  -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF \
  -DSDL_KMSDRM=ON -DSDL_KMSDRM_SHARED=ON -DSDL_OPENGLES=ON \
  -DSDL_ALSA=ON -DSDL_ALSA_SHARED=ON -DSDL_DEPS_SHARED=ON \
  -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_OPENGL=OFF -DSDL_VULKAN=OFF \
  -DSDL_PULSEAUDIO=OFF -DSDL_PIPEWIRE=OFF -DSDL_JACK=OFF -DSDL_SNDIO=OFF
cmake --build build/r36s/sdl --parallel 1
cmake --install build/r36s/sdl
python port/linuxarm64/build_linuxarm64.py --r36s-sysroot "$REKHCOM_R36S_SYSROOT" \
  --r36s-sdl /ruta/SDL-focal --jobs 1
python port/linux/r36s/package.py build/ReKHCoM-r36s-aarch64-alpha.zip \
  --sdl-license /ruta/SDL/LICENSE.txt
```

El builder usa los volcados locales verificados que requiere el proyecto para generar los tres invitados regionales. El empaquetador usa una lista explícita de ejecutables e incorpora los recursos comunes. El ZIP sólo incluye las instrucciones dentro de `roms/`. La auditoría revisa clase ELF, arquitectura, intérprete, dependencias y versiones de símbolos de glibc hasta 2.31, y la cabecera ARM64 ILP32 de cada invitado.

La comparación de rendimiento sigue Opti36S. Usa el mismo volcado, escena, entradas y opciones, mide p95/p99 de presentación y memoria pico en frío y en caliente, y separa los resultados de QEMU y del equipo físico. Registra firmware, kernel, panel, backend SDL, renderer, dispositivo de sonido y SHA-256 del ZIP. [Opti36S](https://github.com/MatyMeatBoy/Opti36S) · [PortMaster](https://portmaster.games/contribute/porting/build-environments/)
