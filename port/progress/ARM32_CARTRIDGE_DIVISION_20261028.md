# ARM32: integración de división de cartucho — piloto 28/10/2026

Hijo `post22-arm32-division-lowering`, base `955eb92c` (#435). La auditoría
padre conserva el oráculo Thumb192 y los prototipos84; los resultados originales
ARM32 no respetaban siempre el resto cero. No se atribuye a esto el cierre S22.

## Productores acotados

- `build_linux.py`: sólo `cc_game` para armhf/Anbernic. i386 y `cc_port` conservan
  su compilador directo. Los objetos del cartucho dependen de ambos scripts.
- CMake Android: sólo `game_<region>` OBJECT en `armeabi-v7a`, mediante
  `C_COMPILER_LAUNCHER`. Launcher/importador, SDL, port y el retorno temprano
  arm64 no se envuelven; Android x86 conserva su compilador anterior.
- El adaptador mantiene los argumentos reales, incluidos response files opacos.
  Exige `-c fuente`, `-o objeto`, `-MF deps` explícitos fuera del response file:
  una forma no soportada falla, no se adivina ni devuelve éxito con un objeto viejo.
  Frontend O2 con `-disable-llvm-passes`, lowerer compartido **sin cambios**, luego
  backend O2. Las dependencias de headers proceden del C original, no del IR.
- Hash de adaptador+lowerer en el comando; CMake añade OBJECT_DEPENDS y
  CMAKE_CONFIGURE_DEPENDS. Los dos compiladores terminan antes de publicar objeto
  y dependencias. i64/vector y poison literal del frontend siguen fuera de alcance.

## Evidencia efectivamente ejecutada

- RED/GREEN del grafo armhf y control i386: dos tests; los cuatro tests del
  lowerer compartido también pasan. El CMake original generado no usaba lowerer.
- Ninja real armhf: `acgtrans.c` y `bg_anim.c` compilan, la pasada caliente no
  hace trabajo. Clang23, ARMv7/Thumb/NEON/hard-float y sysroot existente.
- CMake real NDKr30/Clang21: configure offline, response files forzados reales,
  los mismos dos game TUs compilan y la pasada caliente no hace trabajo. Reglas
  launcher/SDL/rekhcom sin adaptador, `game_eu` con él. No se recompilaron las SDK.
- Dos ejecutables diskless QEMU, con **los flags de game extraídos de esos
  productores reales**: Android softfp y armhf hard-float pasan 192 DIV/REM
  cada uno y cinco controles de signo/branch, evaluación única, sizeof/punteros,
  direcciones fijas e int/float callbacks. Las direcciones fijas se comparan;
  no se accede a un bus GBA real. Se conservaron los atributos ELF32/ARM.
- Cuatro fallos dirigidos (frontend, namespace del lowerer, backend y fingerprint
  obsoleto) devuelven error y conservan los bytes previos de objeto/depfile.
  El primer intento QEMU falló sólo en el driver semihosting ARM compilado Thumb;
  el segundo usa driver ARM y objetos de productor Thumb, sin cambiar producto.

## Antes de dar el hijo por terminado

Por cambio expreso del usuario a revisar OptiPort, se entrega **draft** sin full
build/APK/paquete ni hardware. Pendientes: presión de invalidación automática
por cambio de script/header en ambos grafos, configuraciones negativas x86/arm64,
y revisión independiente del adaptador antes de ampliar compilaciones.
No hay ELF de launcher, APK diagnóstico412 ni bytes publicados v2.2 modificados.
Evidencia: `pruebas/post22-arm32-division/` (fuera del repositorio).

## Cierre 02/11 (Claude, Codex inactivo; usuario: "integra todo")

- Rebasado sobre main `4551bbf26`. Pasan `test_arm32_division_producers` (2), `test_cartridge_division_ir`
  (4) y el oráculo Thumb `test_cartridge_division --rom` (192 controles DIV/REM, cero y overflow).
- Full build real Anbernic armhf EU (270 pasos) con el adaptador: las 391 llamadas a
  `__aeabi_(u)idiv(mod)` del código de juego quedan detrás de una rama condicional de su propio
  helper (`__rekhcom_gba_*32`: divisor 0 → 0, como la GBA y el PC); el compilador pliega la guarda
  en `cmp/beq`, `cmp #1/blt` o `adds/bhs` cuando ya conoce el signo.
- Sin QEMU en esta máquina: la ejecución ARM real del juego queda para el reportero/dispositivo.
