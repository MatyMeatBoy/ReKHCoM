# Canvas HUD: fallo de reserva antes de supresión PPU

Claim `hud-canvas-preflight`, owner `device_profiles_sol`, independiente de #346.

## Mecanismo previo demostrado

`Present` llama `GrowPixelBuffer` para el canvas HUD incluso con `hdHud=0`.
Un fallo del allocator termina en PortFatal. Para entonces `PortPpuBeginFrame`
ya activó `sHudLayer`; los sprites originales se omitieron del mundo.
Saltar únicamente warm-up o desactivar `hdHud` no conserva el HUD.

La recuperación debe resolver readiness antes de `sHudLayer`, conservar
recursos/dimensiones anteriores si falla, y bloquear sustituciones HD/texto
sólo del HUD cuando el canvas no está disponible. No cambiar preferencias.
Un fallo persistente debe quedar cacheado por dimensiones y generación; una
invalidación explícita o cambio de dimensiones permite volver a intentarlo.
No afirma recuperar todo OOM esencial ni cualquier fallo GPU del proceso.

## Evidencia ligera

`python port/tools/test_hud_canvas_preflight.py`

- Selección PPU, guardas nativas/HD/texto y consumer de Present extraídos de
  producción, sin reimplementar sus condiciones. Decoder `ObjColor` real,
  fixture sintética multicolor de64píxeles. RED original exit77 al mantener
  `sHudLayer` ante fallo. Estado configurado nunca se sobrescribe.
- Fallos persistentes por1000cuadros, sin llamadas nuevas al allocator ni
  spam. Mantener puntero/contenido/capacidad anterior; bajar resolución y
  recuperar por invalidación/generación.
- Fixture independiente incluye el archivo PPU completo vía
  `hud_canvas_ppu_conformance.c`/`ppu_conformance.c`. Con canvas fallido y
  HDHUD/cartas/texto activados, framebuffer320x180 byte-idéntico al control
  cartridge con HD desactivado. Recuperar readiness separa nuevamente el HUD.
  Recursos HD y escenarios de fondo ajenos son stubs, no sprites importados.

Syntax-only native32 SDL y x32 freestanding: tres fuentes modificadas PASS.
Regresión de invalidación HUD PASS, incluida señal de retry explícito.

## Alcance CPU y límite GPU

Este cambio prepara únicamente `sHudBuf`. La creación/ownership de la textura
HUD sigue en `Present`, sin alteraciones: el fallo de esa textura todavía no
tiene fallback completo y es otro mecanismo. No atribuirle recuperación GPU.
El buffer anterior y las preferencias se conservan si falla la reserva nueva;
la transacción puede necesitar pico de memoria antiguo+nuevo hasta commit.

El reset durante pausa pertenece a la siguiente matriz GPU: puede invalidar
texturas mientras sigue vigente un frame PPU congelado. No redibujar PPU
arbitrariamente para repararlo: `PortPpuRenderFrame` ejecuta HBlank/DMA/IRQ,
no sólo composición de píxeles. No se cambia ese contrato aquí.

## QA GPU pendiente del coordinador

En copia aislada, inyectar fallo sólo en la reserva candidata del canvas,
con resolución nueva mayor que la pareja anterior; comprobar HUD nativo
completo, nunca mezcla de pilot/labels ni silencios de sprites. Mantener
preferencias y memoria anterior, contar intentos y logs durante1000cuadros.
Recuperar por reducción de resolución/invalidación explícita de recursos.
Inyectar además fallo de textura como prueba separada del mecanismo GPU previo;
no contar ese caso como resuelto por este PR.
Comprobar entrada/salida de Opciones y reset durante las fases de fade, además
de CPU: crecer el buffer world/composite sigue otro contrato esencial.
No ejecutar ni escribir instalación Quiver. No se han ejecutado juegos,
GPU ni builds completos durante este carril.
