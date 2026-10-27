# MsCharge: hermano del divisor cero de #386

`MsChargeDraw` calculaba `ceil(tabCount/3)-4` y dividía por ese rango si
`scroll<=range`. Diez, once o doce entradas dan rango0 y scroll0 entra en la
división. `mode_ms_charge_0` inicia la pestaña All (4), construye la lista y
pone scroll0; la lista cuenta tipos `(kind,premium)` de cartas sueltas.
Es un estado válido, también alcanzable después de vender cartas o cambiar
de pestaña. No se afirma que el reportero de #386 entrara a este menú:
su fallo original está en MapInspect, atendido en otro carril.

La guarda local bajo `PLATFORM_PC` exige rango positivo y scroll dentro de0..range. Una sola
página, lista vacía o estado transitorio fuera de rango conserva el indicador
en y40. Las posiciones válidas de listas desplazables conservan su fórmula.
No cambia cartas, cantidades, venta, guardados, ABI ni el helper global DIV0.
La condición original se conserva sin `PLATFORM_PC` para la versión GBA.

## Regresión dirigida

```
python port/tools/test_ms_charge_scroll.py --work <directorio privado de evidencia>
```

El arnés compila explícitamente con `-DPLATFORM_PC` y extrae completos y sin reescribir `MsChargeDraw`,
`GetMsChargeTabCount` y las declaraciones locales de producción. Usa el tipo
real MsCard; dibujo, animación y texto son dobles controlados. Observa la
posición y el número de envíos del indicador real. No enlaza SDL ni inicia
juego, GPU o audio; cada ejecución crea un subdirectorio nuevo.

- RED en main8d495811e: count10/tabAll/scroll0 termina con `0xC0000094`.
- GREEN:146.243 casos, cantidades0..286, cinco pestañas y scroll-1..98,
  incluyendo extremos válidos y una posición posterior al final; salida0.
- Evidencia privada: `pruebas/64bit-crash-ledger/ms-charge/`
  `ms-charge-hb3fbkd9/result.json` (RED PC) y
  `ms-charge-bwch73nm/result.json` (GREEN PC), con hashes del código y funciones.
- Sin `PLATFORM_PC`, los tokens de `MsChargeDraw` preprocesados coinciden con
  main8d495811e (`nonpc-equivalence.json`). No se recompila el binario GBA.

286 es la capacidad de la lista: se exploran también combinaciones sintéticas
de límite, sin afirmar que todas ocurran en una partida natural. El recorrido
completo del menú y el paquete combinado corresponden al coordinador; esta
prueba no demuestra todas las escenas, dispositivos ni arregla el informe
secundario de excepción del host Windows64.

Auditoría hermana de sólo lectura: Status ya protege rango0; Deck y Exchange
usan DIV0 local; MapSelect exige lastPage>0 en PC. Eso no cierra toda la
estabilidad64. Ledger privado acotado:
`pruebas/64bit-crash-ledger/{README.md,ledger.json,source-snapshot.json}`.
