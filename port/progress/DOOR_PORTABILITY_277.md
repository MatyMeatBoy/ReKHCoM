# Puerta de evento: portabilidad de ROM y paletas — 27/10/2026

La foto de #277 corresponde a una puerta de evento de Ciudad de Paso. Pasar la puerta del castillo y el tutorial no prueba ese recorrido. No se atribuye el problema a HD ni a falta de RAM: la reproducción siguiente tiene todos los extras HD desactivados.

## Fallos reproducidos

1. Linux x86-64, host `rekhcom64`, guest EU anterior de `fix/v21-bugs`: GPF, salida 139, IP `0x100bd9c9`. `LoadMapSelectKindPalette` ejecuta `movaps` sobre `gMapSelectGridPalettes+0x20`, dirección ROM `0x0954bfa4` (alineación 4, no 16). El ABI SysV x32 permite al compilador asumir 16 para ciertos arrays extern sin anotación. El guest anterior tiene tres accesos SIMD alineados inseguros de esta familia.
2. Después de corregir las declaraciones generadas, el fixture con muchas cartas llega al siguiente fallo: `UpdateMapcardGfx`, lectura de dirección 6 al agotarse paletas. Dos accesos pasan por `BIOS_PTR`, igual que otras rutas del port; `FadeSetPaletteExcluded` ya ignora bancos fuera de rango.

Se conserva el layout de la ROM. El generador anota sólo símbolos fijados a direcciones ROM con su alineación real; no se desactiva SIMD globalmente. Los builds existentes ejecutan `--ensure` para actualizar cabeceras cacheadas sin volver a codificar la ROM.

## Verificación

- Harness de declaraciones reales: alineaciones 1/2/4/16, declaración nativa intacta y actualización de caché sin ROM. Repetir no modifica los archivos.
- Auditor del ELF: guest anterior, tres accesos SIMD a ROM mal alineados; guest corregido, cero dentro del conjunto de instrucciones auditadas.
- Harness de `UpdateMapcardGfx` real: paletas válidas, mezcladas y agotadas; sentinel BIOS y bancos válidos.
- Fixture de presión Linux: llega al selector sin el GPF ni acceso NULL; salida 0.
- Entrada completa con inventario mínimo, Windows EU y VM Linux x86-64 (1 GiB, un CPU): ambos llegan a `mode_battle` en frame 2239 y progreso `world=10 room=0` en 2800; salida 0 al terminar 3000 cuadros. Build nativa y guest EU pasan.

Recorrido reproducible de entrada completa:

```
KHCOM_MODE=60:room:10901,120:fieldfreeze,200:doorpose:3,2800:progress
KHCOM_KEYS=240-243:A,420-423:A,620-623:A,820-823:A,1200-1203:RIGHT,1300-1303:RIGHT,
```

Añadir pulsos A de tres cuadros desde 1500 hasta antes de 2990, cada 120; `KHCOM_QUIT=3000`, `KHCOM_FAST=1`, `KHCOM_TRACE_MODES=1`. `doorpose:3` usa una carta roja y llaves; `doorpose:2` conserva presión de cartas y añade llaves. Son fixtures opt-in, no cambios al inventario normal.

Evidencia local: `build/linux-door277-final/{run277,kh_com}.log` y `build/door277-native-key-only/kh_com.log`. Las capturas raw del guest salen negras y no se usan como evidencia visual.

## Límites

Esto reproduce y elimina dos fallos concretos en el recorrido de la puerta. Falta confirmación en el Steam Deck del reportero. No declarar #277 cerrado todavía.

#272 es otra ruta: evento 74 USA y mensaje final en Android. El guest USA anterior pasa ese evento en Linux con HD desactivado (mapa en frame 3178, batalla en 5906, salida 0 a 9000). Las guardas de ventanas/paletas se entregan por separado; no equivalen a reproducción de #272. Tampoco se ha reproducido el cierre del usuario al volver a Monstruo.
