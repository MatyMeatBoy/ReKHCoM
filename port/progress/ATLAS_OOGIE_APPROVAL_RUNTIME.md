# Oogie: aprobación de Atlas y activación real (29/10/2026)

El modo normal no activaba la arena aprobada. `oogieHall` seguía usando
`PortArenasV2()` (sólo preview=1), aunque Atlas había aprobado su versión.
Se cambia únicamente esa compuerta por `PortArenasApproved()`. Off=0 sigue
recuperando el fondo original y el matcher sigue exigiendo el mapa de Oogie.
No se cambia el arte ni se promueven otras arenas.

## Cadena comprobada

- Familia Atlas: `b7df95116b0543719423d2b29c7aa17f`.
- `approved_version` y `build_version`: `aa913fe773c4446eb5412c3115e222c7`,
  estado approved; SHA PNG `e894551172e73b53e029417430306beba07948db59c4625f487490f882331ed2`.
- Receta implementada: PR107, merge `6bcb39da85e9935b7aac0890c31efe59986d4fb2`.
  `port/src/port_oogie.c` conserva exactamente el blob Git
  `7238dfda8b7d2c400bd838057b0db564f89f219c` de ese merge en main c44f679.
- Las hijas `e90c2d6d475447458a05db85276db6bb` y
  `677876e57b1441dc856caa8847f84487` siguen pendientes. PR130 está cerrado
  **sin merge**. Sus cambios no forman parte de esta activación.
- No hay tabla de importación ni PNG HD que desplegar para esta arena.
  `PortOogieLayer` reconoce `gBosBoogieBgMaps`; `PortOogieHallPixel` obtiene
  tiles de la ROM y paleta viva. Existencia de una imagen Atlas no activa código.
- Baseline real EU32: default y preview producían fondos distintos; el centro
  GBA era idéntico en frames240/330. La prueba C del gate fallaba con
  `gate=0 expected=1` para default + mapa válido de Oogie.
- Después: seis procesos EU32, SDLdummy/software, un hilo, 1920x1080/3840x1080,
  default/preview=1/Off=0, frames240/330: doce capturas, seis exits0.
  Default=preview pixel por pixel en4/4; centro GBA=Off en4/4.
  A1920, defaultnuevo=previewanterior2/2 y Offnuevo=defaultanterior2/2.
- Prueba CPU: doce combinaciones del gate C real con default/on/off,
  latch válido/inválido y mapa propio/ajeno. RED antes, GREEN después.
- Captura hija de verificación Atlas: `55a0b17eb5f3469dab063b67a6de03da`,
  enlazada a la aprobación exacta. No reclama resolver comentarios posteriores.

## Recursos y límites

La referencia Atlas mide1920x2184:803912 bytes PNG,16773120 bytes RGBA.
Las capturas runtime16:9 nuevas ocupan62615–62809 bytes PNG,8294400RGBA;
las32:9,92776–93228PNG,16588800RGBA. Son pruebas, no texturas cargadas
por el juego. Esta corrección no añade recursos ni cambia resolución/filtros.
Los recorridos tardaron4.89–9.985s incluyendo334frames y escritura PNG;
no son p95/p99 ni demuestran mejora de stutter.

Evidencia local protegida fuera del repo: `pruebas/atlas-v23-oogie/`:
`baseline.json`, `fixed-runs.json`, `runtime-comparison.json`, capturas/logs y
`build-receipt.json` con357objetos/entradas reutilizados y hashes.
Binario de prueba: `509dc4b60d716ec57d7cae0bb70bca6a98f55b2d2d4d7ab4364c87decb6e602d`.
Se compiló PPU propio y se enlazó a objetos EU32 existentes, sin escribirlos;
esos objetos incluyen WIP del diario declarado por el integrador. No es una
build clean ni certifica64/GPU/Android. Primer controlOff anterior al fix
falló antes de entrar al guest por dirección de secciónGBA/ASLR; conserva su
log en baseline-0. El controlOff posterior sí termina0 en ambos tamaños.
La entrega clean debe compilar desde el merge y conservar esta receta activa.
