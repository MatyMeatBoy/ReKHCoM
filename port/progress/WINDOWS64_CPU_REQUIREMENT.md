# Windows64: validar requisito SSE4.1 — 27/10/2026

El builder Windows64 utiliza `port/ps5/build_ps5.py`; sus flags comunes habilitan
`-msse4.1` para el guest x32. Se mantiene esa baseline. Antes, el selector
comprobaba Windows AMD64 y archivos, y el host sólo registraba CPUID en el
diagnóstico: una CPU incompatible podía llegar al guest.

Ahora la misma consulta CPUID y el mismo decodificador del bit19 de leaf1/ECX
sirven al launcher, al diagnóstico y al guard del host. Automático selecciona
32 bits si falta SSE4.1. Forzar64 muestra el requisito de CPU y conserva la
preferencia guardada; los errores de plataforma/archivos mantienen su aviso.
El host invocado directamente devuelve5 antes de abrir/mapear la imagen,
crear renderer o escribir el marcador de entrada. El marcador anterior se
elimina primero. El nuevo código admite fallback pre-entrada si el selector y
el host discrepan; no modifica la política posterior a entrada/crash.

## Evidencia dirigida

Base fresca `aef6b5309a222d7de88a8705322876aa92f8f045`. Se escribió y ejecutó
primero el test del consumidor real `Launch`: con CPU incompatible, intentaba
iniciar64 antes de considerar32. También se compiló el host productivo con un
forced-include exclusivo de test que enmascara SSE4.1 en CPUID: entraba al guest
sintético, devolvía81 y dejaba marcador. Ambos reprodujeron la omisión antes
de implementar el guard.

Después del cambio:

- `test_launcher_cpu_gate.py`: CPU presente/ausente × Auto/Forzar32/Forzar64,
  primer proceso elegido, ausencia de proceso incompatible, aviso SSE4.1,
  preferencia persistida intacta y aviso distinto para archivo faltante: PASS.
- `test_runtime_selection.cpp`: predicados de selección, requisito y retry
  pre-entrada del código5, más clasificación existente de crashes: PASS.
- `test_cpu_requirement.py`: host real con CPU nativa devuelve81/con marcador;
  variante test CPUID enmascarado devuelve5/sin marcador. SRAM/INI/progreso
  sintéticos permanecen byte-idénticos: PASS. No se compiló guest ni usó ROM.
- `test_boot_status.py` existente sobre el host nuevo: errores2/4 sin marcador
  y guest sintético81 con marcador durable: PASS.
- Test existente de ABI/argumentos/mapas/buffers/UTF-8 del host: compilación y
  ejecución rc0. Temporales limitados a la carpeta privada de pruebas.
- Frontend completo Windows i686 `decompui.cpp`: syntax-only con flags reales
  del launcher, SDL3 e ImGui: PASS. No se reconstruyó el launcher completo.
- Preflight de ambos harnesses antes de preparar/escribir/compilar: seis
  pruebas negativas por CLI conservan bytes y árbol de archivos ajenos; sólo
  fixtures sintéticas exactas permiten rerun. Este cambio sólo afecta tests y
  no requirió recompilar hosts ni guest.

Los binarios/logs y `cpu-results.json` quedan en el árbol privado
`pruebas/windows64-cpu-gate`. La única advertencia de compilación del host es
la comparación de punteros preexistente de `stb_vorbis.c`; no se modificó.
Se usaron ventanas ocultas y audio dummy. El SDL dummy de los probes sólo
acredita control de flujo/contratos: no constituye QA de GPU ni audio.

## Límites y seguimiento

Es una corrección de compatibilidad comprobada; **no es un arreglo demostrado
del cierre de Pedro**. Declaró Windows11 x64, sin CPU ni código de excepción.
La CPU de pruebas sí dispone de SSE4.1; el caso negativo usa una máscara CPUID
exclusiva de un binario de test. No se probó hardware antiguo ni se ejecutó
el guest publicado bajo una CPU emulada sin esa instrucción.

No cambian el ABI del bridge, la baseline del guest, arte, guardados, ajustes,
otros hosts ni releases. El host/launcher corregidos aún deben incorporarse a
un futuro paquete autorizado; el paquete2.1 publicado sigue siendo el mismo.

## Rebase antes del PR

La rama se rebasó sobre `f1b51b9c21663aeacdbacd7184b2b2b6baa104e2`
(#376, argumentos de idioma Linux). La comparación de blobs Windows entre el
commit probado `537510fcd` y la rama rebasada no muestra cambios en host,
launcher, helper CPU ni tests. No se reconstruyó ningún guest ni se repitió
la compilación del host por ese cambio documental/de Linux.
