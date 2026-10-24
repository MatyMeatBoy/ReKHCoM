# Linux: PC x86, Steam Deck y Anbernic (18/10/2026)

## Por qué 32 bits

El núcleo necesita punteros de 32 bits (las estructuras de la ROM los llevan
dentro: `port/tools/audit_pointer_layout.py`). Linux usa por eso i386 (PC y
Steam Deck) y armhf (portátiles ARM). Cada paquete lleva su propio cargador
(`lib/rekhcom`, el ld-linux renombrado para que el proceso se llame así),
glibc, SDL3 y las bibliotecas cliente de X11/ALSA/PulseAudio/udev/D-Bus de
Debian 12: no hace falta ningún paquete de 32 bits en el sistema. El driver de
GPU del sistema (64 bits) no se puede cargar, así que SDL dibuja con su
renderer software.

## Piezas del backend

- `port/android/native/port_memory_android.c`: reserva POSIX del espacio GBA
  (0x02000000-0x0E010000), compartido con Android.
- `port/linux/platform_linux.c`: secciones `khdata/khbss`, tabla Shift-JIS
  completa (generada al compilar), log de crash, stubs ARM.
- `port/src/port_biosfault.c`: la emulación de accesos NULL/bus y división por
  cero (i386) funciona con `sigaction` además del manejador de Windows.
  En ARM, como Android: `BIOS_PTR` y `__aeabi_idiv0` devuelve 0.
- `port/src/port_state.c`: quickstates en Linux (ejecutable con base fija
  0x10000000, sin PIE).
- `port/linux/fbdev_linux.c`: con `KHCOM_FBDEV=/dev/fb0`, SDL usa su driver
  `offscreen` y cada fotograma se copia al framebuffer (32 bpp o RGB565).
  Para portátiles cuyos drivers de GPU son solo de 64 bits.
- Lanzadores: `port/linux/ReKHCoM.sh` (detecta la región por la cabecera de la
  ROM en `roms/`) y `port/linux/anbernic/ReKHCoM.sh` (formato PortMaster,
  gptokeyb para salir con SELECT+START).
- Guía de usuario incluida en los paquetes: `port/linux/LINUX.md`.

## Compilar (desde Windows, sin WSL)

Una vez:

    python port/tools/linux_prepare.py

Descarga los paquetes Debian 12 (i386 y armhf) a `D:/01Claude/ReKHCoM-build/linux`,
genera el pkg-config sustituto y los toolchains de CMake y compila SDL3
(compartida) y FreeType (estática) para cada arquitectura.

Después:

    python port/tools/build_linux.py --arch i386 --versions eu us jp
    python port/tools/build_linux.py --package v1.9.3
    python port/tools/build_all.py --platform all --package --release-version v1.9.3

`--package` deja en `build/releases`: `ReKHCoM-vX-linux-x86.tar.gz`,
`ReKHCoM-vX-steamdeck.tar.gz` (mismo programa con `kh_com.ini` en pantalla
completa) y `ReKHCoM-vX-anbernic-alpha.zip`.

## Pruebas

`port/tools/linux_vm_test.py` arranca QEMU (imágenes Debian 12 nocloud en
`D:/01Claude/ReKHCoM-build/linux/vm`), pasa la carpeta como un tar usado de disco y
devuelve archivos por la consola serie (la UART pierde caracteres si se
escribe deprisa: el script envía en trozos).

Hecho (VM Debian 12 x86_64 sin bibliotecas de 32 bits, WHPX):
- EU/US/JP arrancan con el paquete autocontenido (renderer software).
- Sala de campo con movimiento; quickstate guardado y cargado en otra
  ejecución con la misma imagen.
- Combate con truco de Sora: las dos escrituras NULL de
  `ClearStockedCardSlots` (`@bug`) se emulan como en Windows
  (`bios: write 00000007`), el combate sigue y el proceso sale con 0.

- Paquete final `linux-x86.tar.gz` extraído tal cual: lanzado desde otra
  carpeta, detecta la ROM JP por la cabecera (archivo `my dump.gba`), juega
  sala y combate y sale con 0.

Hecho (VM Debian 12 arm64, Cortex-A53 emulado, virtio-gpu con /dev/fb0):
- El paquete armhf corre en el kernel de 64 bits, abre /dev/fb0 (1280x800,
  32 bpp) y la imagen del juego llega al framebuffer escalada y con los
  colores correctos (volcado comprimido revisado).
- Hallazgo: en kernels ARM64, lanzar el cargador a mano (`ld-linux prog`)
  pone el heap (brk) en 0x021xxxxx, dentro del espacio GBA. Por eso el juego
  se enlaza con `lib/rekhcom` como intérprete relativo y el lanzador entra en
  la carpeta antes de ejecutarlo; el heap queda detrás del ejecutable.
- Rendimiento NO medible ahí: la CPU ARM va emulada por software (600
  fotogramas en 5,5 min).

**No probado en hardware real**: escritorio Linux con X11/PulseAudio de verdad,
Steam Deck, ni ninguna Anbernic.

## Pendiente

- Probar en un PC Linux real (X11 y XWayland, sonido) y en una Steam Deck.
- Anbernic: probar en una RG34XX real (Knulli/muOS/ROCKNIX): kernel con
  soporte de 32 bits, `/dev/fb0`, orientación del panel, mandos (mapeo
  SDL/PortMaster), rendimiento del A53 en modo AArch32.
- Texto HD en japonés: la fuente CJK se busca en `KHCOM_CJK_FONT`, en rutas
  habituales (Noto CJK, Takao, IPA, Droid, WenQuanYi) y con `fc-match :lang=ja`;
  sin ninguna, el HD japonés sigue desactivado. Falta confirmarlo en una
  distro/Steam Deck real.
- No hay launcher gráfico en Linux (el de Windows usa APIs de Windows); los
  ajustes están en el menú de opciones del juego.
