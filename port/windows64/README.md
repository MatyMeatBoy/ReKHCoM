# Windows x64 (alpha)

Host nativo PE x86-64 con SDL3 x64 y driver GPU normal de Windows. El juego
conserva la imagen x32 compartida con Linux64/PS5: instrucciones x86-64 y
punteros de 32 bits; no se ensanchan estructuras del juego.

```powershell
python port/windows64/build_windows64.py --regions eu us jp --jobs 1 --test
# Solo host y pruebas ABI (sin recompilar imÃ¡genes):
python port/windows64/build_windows64.py --host-only --test
build/windows64/out/rekhcom64.exe build/windows64/out/guest-eu.bin --data RUTA_DATOS
```

El directorio de salida contiene `rekhcom64.exe`, `SDL3.dll` x64 y las imÃ¡genes
`guest-<region>.bin`. `--host-only` conserva las imÃ¡genes ya presentes.
Para un paquete de usuario se excluye `test_windows64.exe`; nunca se empaqueta
ROM, partida ni configuraciÃ³n de pruebas. Mantener Windows32 como alternativa.
Se requieren ROM propia y recursos de datos con la misma disposiciÃ³n del port
Windows32. La variable `KHCOM_HOST_HIDDEN=1` oculta la ventana de los smokes;
`KHCOM_HOST_SHOT=400` captura el frame 400 en la carpeta de datos.

Cada direcciÃ³n de funciÃ³n de la tabla apunta a un wrapper tipado `sysv_abi`.
Clang genera el cambio hacia Microsoft x64 para SDL, libc y Win32, incluido
shadow space y preservaciÃ³n de registros; el tipo de `boot` tambiÃ©n es SysV.
La reserva baja usa `VirtualAlloc` a direcciÃ³n exacta y falla sin reemplazar
reservas existentes. Directorios usan FindFirstFile/FindNextFile de Win32.

## VerificaciÃ³n del hito (27/10/2026)

- CompilaciÃ³n del host y prueba ABI ejecutadas en Windows x64 con LLVM MinGW.
- Prueba ensamblador de RDI, RSI, XMM6, XMM15 y canario de pila a travÃ©s de
  llamada MS64â†’SysV; argumentos enteros/flotantes y retorno double; quinto
  argumento entero hacia Microsoft x64; wrappers math, archivo, directorio,
  entorno y thread. Reserva baja ocupada falla y conserva contenido.
- Imagen EU previamente compilada del checkout crash-diagnostics: puerta
  `60:room:10901,120:fieldfreeze,200:doorpose:0`, A 240..242, salida a 500,
  driver host `direct3d11`, rc 0. Captura 400 revisada: Room Synthesis correcto.
- Evento 107, entrada por campo â†’ Event frame121 â†’ mode_battle frame1182;
  salida frame1800 rc0. Es smoke parcial, no cadena completa ganada.
- Monstro: `KHCOM_BATTLE=121:5:0`, A frame60, mode_battle frame62,
  salida frame500 rc0 (Shadow 100).
- Estos smokes usaron ROM propia, audio dummy, directorios aislados sin HD
  ni fuentes. No demuestran paridad HD completa. Repetir con imÃ¡genes finales
  y recursos completos antes de publicar la alpha.

## LÃ­mites conocidos

La mÃºsica HD usa `shared/hd_music_host.h`; pelÃ­culas HD devuelven 0 para que
el invitado reproduzca el vÃ­deo ROM. Falta conectar Media Foundation x64.
UDP devuelve sin socket (conserva alternativa cable del juego). Hilos PPU
usan un solo nÃºcleo. No instala manejador de recuperaciÃ³n de fallos del
port Windows32: un acceso invÃ¡lido sigue siendo un crash observable.
Pendiente incorporar tambiÃ©n aquÃ­ cualquier cambio posterior del host Linux64
(p. ej. renderer reset), pues este host constituye una implementaciÃ³n separada.
