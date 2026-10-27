# Previews: contrato de cancelación y obsolescencia — 27/10/2026

Carril `optiport-v22-async`, alcance acotado a cancelación/cierre y selección
vigente. Fuente inicial `61aee4c40fbfded53ba94ad4de978f16907b4939`;
se revisaron [#342](https://github.com/MatyMeatBoy/ReKHCoM/pull/342), el productor
actual y sus consumidores. OptiPort verificado en `2d6827d4`.

No se encontró un defecto productivo en estos interleavings. Este PR añade
una regresión de contrato; no cambia UI, worker, renderer, caché ni decodificador.
No toca los archivos de #379 ni duplica SDKs/builds del juego.

## Evidencia dirigida

`test_preview_cancellation_contract.py` compila las funciones productivas de
preview/cancel/poll, shutdown y `PortUiForceClose`. El trabajo del worker se
completa explícitamente desde la fixture, con stubs de los límites SDL. No
enlaza SDL ni crea hilos, superficies, ventanas, archivos multimedia o GPU.
El reloj de debounce es un entero controlado; no hay sleeps ni cronometraje.
Se esperó la señal del coordinador antes de compilar/ejecutar, después de la
ventana de medición de Claude.

Ocho escenarios comprueban:

- Cerrar mediante `PortUiForceClose` no llama join del decoder pendiente;
  la selección/textura se invalidan y su resultado posterior no reabre nada.
- Reabrir la misma escena descarta el trabajo de la visita anterior.
- HD pendiente seguido de GBA conserva el proveedor GBA seleccionado.
- Ciclo HD→GBA→HD de la misma escena descarta el primer serial HD.
- Cambiar rápidamente a otra escena HD mantiene un worker; el resultado
  aplicado corresponde a la última selección.
- Una generación/ruta nueva pedida inmediatamente después de terminar el
  worker viejo devuelve respaldo vigente, no el resultado de la ruta anterior.
- Decoder o creación del hilo fallidos conservan respaldo y liberan el job;
  no se reintenta en bucle la misma selección.
- Shutdown final del proceso sí hace join deliberado del trabajo pendiente
  antes de liberar memoria. Ese contrato difiere del cierre del menú.

Dos controles negativos modifican únicamente copias privadas del código de
prueba: retirar el guard de serial y añadir join incondicional al cancel.
Cada copia debe fallar en su aserción correspondiente. No representan bugs
observados en el producto. Las aserciones del harness terminan por stderr con
código97 para evitar diálogos CRT en Windows.

```powershell
python port/tools/test_preview_cancellation_contract.py --work RUTA_PRIVADA --cc CLANG --negative-controls
```

Cada ejecución usa un subdirectorio nuevo de `--work` y conserva C generado,
ejecutable, log y manifiesto con SHA de fuente/artefacto. No toca SRAM, INI,
imports ni la instalación del jugador. Evidencia local bajo
`pruebas/v22-async-contracts`; el manifiesto identifica el commit probado.

## Qué sigue pendiente

Esto valida decisiones de los consumidores C bajo una secuencia determinista,
no el scheduling real, la semántica de atomics de SDL, latencia de cierre,
cancelación interna de Media Foundation, lectura/PNG sidecar, presión de memoria
ni un apagado con I/O bloqueado. Los PNG se mantienen ausentes en esta fixture.
El dibujo/subida de texturas, calidad y dispositivos requieren evidencia propia.

No se declara cerrado el bloque completo de asíncrono/precarga de PLAN_V22.
Siguen separados frío/caliente, entrada→primer respaldo→HD final, presupuesto
real y p95/p99. Los tests previos con SDL/hilo lento cubren otros límites;
esta prueba no convierte sus tiempos de harness en rendimiento del juego.
