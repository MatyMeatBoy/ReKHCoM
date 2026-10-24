# Android de 64 bits (21/10/2026)

Pedido del usuario: Android con compatibilidad completa, replicar los errores, batería completa
y arreglarlos. Este archivo es el estado y el plan para retomarlo (otro Claude o Codex).

## Por qué

La app actual es solo de 32 bits (armeabi-v7a; x86 para emuladores) y reserva el espacio de la
GBA en sus direcciones reales (0x02000000-0x0E010000, `port/android/native/port_memory_android.c`).

- #22 y #34 ("Cannot reserve the GBA address space"): en algunos teléfonos (Pixel según un
  reporte, Huawei Mate XT con HarmonyOS 4.2 / Android 12) algo ya ocupa ese rango. Los chips
  nuevos sin AArch32 corren las apps de 32 bits con un traductor binario, y eso explica que pase en
  varias marcas; los Pixel 7 en adelante directamente no ejecutan apps de 32 bits.
- Las imágenes de emulador de Google para PC (x86_64) de Android 13, 14 y 15 ya no traen 32 bits
  (`ro.system.product.cpu.abilist32=` vacío): el fallo no se puede reproducir ahí. Solo la de
  Android 7 (x86) corre la app actual.
- Otro port de GBA (Pokémon Emerald multiplataforma) tiene la misma limitación: Android solo ARM de
  32 bits.

Proyectos parecidos (revisados 21/10): el port Android de Crash Team Racing (igawa6/ctr-native-android)
compila en 64 bits (LP64) y traduce los punteros de 32 bits de las estructuras de PSX con un registro
de regiones; el de Pokémon Emerald sigue en 32 bits. El issue de Flutter #86855 (apps de 32 bits que
caen en teléfonos de 18 GB) era un desborde de enteros en su motor: no aplica. Nuestra vía (imagen
ILP32 como en Switch) evita reescribir el juego.

Paliativo ya publicado en main (0b5fce2f4): la librería reserva el rango al cargarse (constructor),
antes que drivers y heap de Java, y el mensaje de error lista qué hay en el rango.

## Cómo

Lo mismo que Switch (`port/switch/README.md`): el juego y la capa del port como imagen AArch64 con
punteros de 32 bits (clang `arm64_32`, Mach-O), con el espacio GBA reubicable (`PORT_GBA_RELOC`, las
listas de punteros de la ROM en `port/data/rom_pointers_*.inc`), y un host de 64 bits que la carga
y le sirve el puente (`port/ps5/shared/bridge_list.h`). Probado en Switch/Vita: título, menús y
combate idénticos píxel a píxel en la VM ARM64.

Diferencias para Android:
1. Dirección de la imagen: en procesos de 64 bits ART usa los 4 GB bajos (heap en 0x12c00000, boot
   image ~0x70000000). La imagen no puede ir en 0x10000000 como en Switch: medir los mapas de
   procesos de 64 bits en los emuladores y elegir una base libre (alta dentro de los 4 GB bajos),
   reservada desde el constructor de la librería.
2. Host: `port/android64/` con SDL3 real (64 bits): gfx con `SDL_RenderGeometry` (los HostVertex
   del puente son triángulos), audio con SDL audio stream, mandos SDL, archivos en el directorio de
   la app, y los controles táctiles (`touch_android.c`) del lado del host, sumando botones al pad.
3. APK: arm64-v8a (teléfonos) + x86_64 solo si hace falta para pruebas (las imágenes x86_64 de
   API 30+ traducen arm64: `abilist64=x86_64,arm64-v8a`).
4. Mantener la de 32 bits como respaldo hasta que la de 64 bits pase la batería.

## Batería

- Escritorio (atrapa lo que cierra Android: `bios:` NULL/BIOS/div0): `debug_kit.py run --build
  --versions eu,us,jp` + `report` (combates, Riku, eventos, 16:9, salas, monkey, escenas).
- Android: `test_android_scenarios.py` en emuladores Android 13/14/15 (x86_64, perfiles Pixel y
  genérico) con el APK de 64 bits.

## Hoja de ruta (en orden; marcar en "Estado" al terminar cada paso)

1. **Emuladores.** SDK: `toolchain/android/sdk` (JDK en `toolchain/android/java/jdk-17.0.20.1+1`,
   `JAVA_HOME` apuntando ahí). AVDs en `D:/01Claude/ReKHCoM-build/android-avd` (`ANDROID_AVD_HOME`). D: anda
   justo de espacio: `disk.dataPartition.size=4G` en el `config.ini` del AVD y `-partition-size 4096`.
   Crear con `avdmanager create avd -n <nombre> -k "system-images;android-<API>;google_apis;x86_64"
   -d <perfil>` (el `.bat` parte los `;` de la línea de comandos: para `sdkmanager` usar
   `--package_file`). Perfiles: `pixel_6` (teléfono grande), `pixel_tablet` o `Nexus 10` (tablet),
   `pixel_fold` (plegable, como el Mate XT de #34), `small_phone`. Arrancar sin ventana ni audio:
   `emulator -avd X -no-window -no-audio -no-snapshot -gpu swiftshader_indirect`.
2. **Dirección de la imagen.** Con el emulador arriba, leer `/proc/<pid>/maps` de una app de 64
   bits (p. ej. `com.android.settings`) en API 33/34/35 y en un perfil arm64 traducido; anotar acá
   los rangos libres por debajo de 4 GB. Elegir una base que esté libre en todos (candidata
   0xA0000000-0xE0000000) y cambiar `BASE` de una copia del enlace de Switch.
3. **Imagen del juego.** `port/switch/build_switch.py guest` ya genera `guest-<región>.macho`
   (arm64_32, `PORT_GBA_RELOC`, Cortex-A57). Para Android: misma compilación con
   `-pagezero_size <base>` (o un `build_android64.py` que importe `build_switch` y solo cambie la base
   y `-mcpu` genérico ARMv8.0), sin `PORT_SWITCH`.
4. **Host.** `port/android64/host/android_host.c`, a partir de `port/switch/host/switch_host.c`:
   - carga del Mach-O igual que el host de prueba Linux (mmap fijo en la base, desde el constructor
     de la librería para ganarle a ART);
   - gfx con SDL3 `SDL_RenderGeometry` (los `HostVertex` del puente son triángulos) en vez del
     rasterizador por software; audio con `SDL_AudioStream`; mandos SDL; archivos bajo
     `getFilesDir()` (el argumento `--data-dir` que ya pasa `GameActivity.java`);
   - controles táctiles: mover `port/android/native/touch_android.c` al host y sumar sus botones al
     `HostPad`; lo que use de la configuración se lee de `kh_com.ini`;
   - JNI de `platform_android.c` (2 llamadas) al host.
5. **APK.** `build_android.py --abi arm64-v8a` que empaquete el host de 64 bits y las imágenes en
   `assets/`; un solo APK con armeabi-v7a (la de 32 bits actual, respaldo) y arm64-v8a: Android
   elige la de 64 bits donde puede.
6. **Batería Android.** `port/tools/test_android_scenarios.py` (arranque, combate, escena, sala,
   guardar/cargar, captura) en cada AVD del paso 1, vertical y horizontal. Las imágenes x86_64 de
   API 30+ ejecutan arm64 traducido (`abilist64=x86_64,arm64-v8a`), así que el APK arm64 corre en
   ellas.
7. **Batería escritorio.** `debug_kit.py run --build --versions eu,us,jp` y `report`; cada sitio
   `bios:` nuevo se arregla con `BIOS_PTR` / `DIV0` / `MOD0` (como #33). `test_events.py` para las
   escenas.
8. **Publicar.** Prerelease (Windows + APK) para que prueben quienes abrieron #22 y #34; responder
   en los issues con el enlace. Estable solo con el OK del usuario.

## Estado

- [x] #33 (cierre antes del combate con Trickmaster): reproducido en PC (EVENT_099, actor NULL en
  task_bos_tm_1 y event_chara_1), arreglado con BIOS_PTR (0b5fce2f4).
- [x] Imágenes Android 13/14/15 x86_64 descargadas (SDK en toolchain/android/sdk).
- [ ] Malla de escritorio completa en EU/US/JP (corriendo 21/10).
- [x] Mapa de memoria de procesos de 64 bits (Android 14 x86_64, Settings y SystemUI, 21/10):
  libre todo lo que hay por debajo de 0x12C00000 (heap principal de ART en 0x12c00000-0x2ac00000,
  objetos grandes de ART en 0x40000000-0x62000000, boot image 0x6f50-0x75fa0000, centinela en
  0xebad6000). Base elegida para la imagen: **0x01000000** (la imagen ocupa ~150 MB, termina antes
  de 0x0b000000), reservada desde el constructor de la librería. Repetir la medición en Android 13 y
  15 y con un APK arm64 traducido.
- [x] Imagen del juego para Android (`port/android64/build_android64.py`, base 0x01000000, PORT_ANDROID64:
  controles táctiles y pausa en segundo plano por las entradas nuevas del puente `touch_poll`/`app_state`).
- [x] Host Android de 64 bits (`port/android64/host/android64_host.c`): SDL3 con GPU (SDL_RenderGeometry),
  audio por stream SDL, mandos, toques, archivos normales o `apk:/` del APK, imagen reservada desde el
  constructor, capturas de prueba `KHCOM_HOST_SHOT`.
- [x] APK único: armeabi-v7a (32 bits, teléfonos viejos) + arm64-v8a (host de 64 bits) + `assets/guest-*.macho`;
  `GameActivity` usa `rekhcom64` donde Android instaló las librerías de 64 bits. Misma firma y paquete:
  actualiza encima y conserva las partidas (verificado: certificado 3a5a17… igual al de v1.9.12).
- [ ] Batería en emuladores (21/10, en curso, Android 14 x86_64 con el APK arm64 traducido, `--slow 6`): EU menús,
  fixtures, jefes; EU jefes con Riku; US/JP menús y jefes. Hasta ahora sin fallos del juego. Arreglos de la
  propia batería: 7 pantallas que `KHCOM_MODE` no abría (tienda, carga, continuar, logo, selector de eventos,
  tutorial de Riku, versus) agregadas a la tabla del arnés (`src/main.c`); `deckuse` ahora con mazo válido;
  `doorselect` fuera de la lista (artefacto del arnés ya conocido en `debug_kit.py`).
- [x] #22 y #34 (y #33, #35 Switch) comentados con la prerelease de prueba v1.9.13-pre2 (21/10): se pidió que
  prueben y dejen modelo y versión de Android; se cierran con su confirmación.
- [ ] **Respuestas a la pre2 (21/10, 16:16 UTC):** Odin 2 Portal anda bien (#22, xsm2). Poco F8 Ultra (#22) y
  Huawei Mate XT, HarmonyOS 4.2 / Android 12 (#34, Xivadas): "The game image could not be loaded" al pulsar jugar.
  Pixel 4 con Android 13 (#22, DrunkECheese) sin resultado todavía. Causa más probable: en teléfonos reales algo ocupa
  0x01000000-0x0C000000 (ART reparte sus bloques de memoria baja desde abajo; en el emulador estaba libre), la reserva
  del constructor falla y el `MAP_FIXED_NOREPLACE` de cada segmento también. **Arreglo escrito (rama, sin probar en
  emulador todavía):** la imagen ya es un Mach-O PIE con su tabla de reubicaciones (`LC_DYLD_INFO_ONLY`, 78.552
  punteros en EU): el host intenta 0x01000000 y, si está ocupada, deja en el log qué la ocupa, busca un hueco libre
  por debajo de 4 GiB (desde 0xF0000000 hacia abajo, que ART llena último), carga la imagen ahí y aplica las
  reubicaciones. El mensaje de error ahora dice la causa en pantalla (los testers no encuentran cómo pasar el log).
  Los estados rápidos de otra dirección se rechazan solos (su firma incluye la dirección de los datos); las partidas
  `.sav` no dependen de ella. Falta: probar en el emulador con `KHCOM_IMAGE_MOVE=1` (fuerza otra dirección) la
  batería de menús y jefes, rehacer el APK y pedir a los testers que lo prueben (con el OK del usuario).
