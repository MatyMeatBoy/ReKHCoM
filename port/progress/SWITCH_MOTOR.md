# Motor nuevo de Switch (plan, 24/10/2026)

Pedido del usuario: Switch a velocidad completa (#35: 20-25 fps antes de presentar por GPU; sin medidas después).
Referencia: ggdrn/KH_COM_VITA llega a 60 fps en Vita con la lógica en un núcleo y la PPU en otros dos (3-6 ms por
cuadro), escala por GPU (vitaGL).

## Publicación (decisión del usuario, 24/10)

La **v2.0 estable sale sin Switch**. La Switch se queda en la última prerelease de la 2.0 (el motor actual). Vuelve a
un release cuando el motor nuevo esté hecho y comprobado (medido en una Switch real). Al armar la v2.0: sacar el
`.nro` de `stage_release.py` MAIN para esa publicación y decirlo en las notas.

## Cómo es hoy

- El juego y toda la capa del port (PPU incluida) son una **imagen invitada** AArch64 de punteros de 32 bits
  (`arm64_32`, Mach-O en 0x10000000) servida por el puente (`port/ps5/shared/bridge_list.h`). El host
  (`port/switch/host/switch_host.c`, libnx) mapea la imagen, presenta las texturas por GPU (SDL2), suena y lee mandos.
- La PPU corre **en un solo núcleo**: el invitado no tiene hilos ni TLS, y `PORT_PPU_THREADS` (filas fuera de la
  imagen GBA repartidas entre núcleos, `RenderOuterRows`) es solo de escritorio porque usa `_Thread_local`.
- El salto de cuadros mantiene la velocidad del juego cuando no se llega a dibujar (`frame skip` en el log).

## Fases

0. **Medir en la consola** (primero, barato): una línea por segundo en el log de las consolas, como la de la Vita de
   ggdrn: µs de lógica, PPU y presentación, cuadros saltados. Con el log de una Switch real se sabe qué parte pesa.
   Sin eso el motor se construye a ciegas.
1. **PPU en varios núcleos en el invitado**:
   - Puente: `thread_start(fn, arg, core)`, `sem_create/post/wait` (libnx `threadCreate` en los núcleos 0-2; el host
     llama la función del invitado con el mismo trampolín que `guest_main`). En el host de prueba Linux, pthreads.
   - Sin TLS: los búferes de línea marcados `PPU_TLS` pasan a un contexto por núcleo (`PpuCtx`) que recorren las
     funciones de dibujo (refactor mecánico de `port_ppu.c`). En PC se comprueba píxel a píxel
     (`compare_ppu_builds.py`, `KHCOM_PPU_VERIFY`, baterías) antes de habilitarlo en consolas.
   - Primero las filas fuera de la imagen (como en PC); después las 160 filas de la imagen guardando por línea el
     estado que usa la PPU (registros, paleta: el siguiente paso también para PC a x1, RENDIMIENTO.md).
2. **Solo si 1 no alcanza**: la PPU en la GPU (deko3d / shaders), proyecto grande.

## Cómo se prueba

- Correctitud: el host de prueba Linux AArch64 en la VM ARM64 (`build_switch.py`, `--shot`), mismas capturas que
  con un núcleo; en PC la misma PPU con contexto por núcleo, idéntica píxel a píxel.
- Rendimiento: solo la consola. Un emulador (Ryujinx) sirve para cazar crashes, no para medir velocidad.
- Lo mismo aplica a la Vita (mismo host compartido, 3 núcleos de usuario).
