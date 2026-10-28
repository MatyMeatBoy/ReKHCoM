# OptiPort 2.2: vista profunda de tirones

Tarea `optiport-v22-profiling`, owner `claude-v22`, 27/10/2026. Pedido del
usuario: «una vista profunda a las heridas donde haya stutters».

## Qué añade

Con `KHCOM_FRAMELOG` (el perfilador ya lo activa):

- `frames.csv` suma columnas por cuadro: `logic_ms` (intervalo entre el final
  del log anterior y el inicio del ciclo), `ppu_ms`, `present_ms` (capas HD incluidas) y los costes
  puntuales de ese cuadro: `hd_loads`/`hd_load_ms` (decodificación y subida
  de sprites HD) y `glyphs`/`glyph_ms` (glifos HD renderizados por primera vez).
- Cada cuadro de más de `KHCOM_STUTTER` ms (25 por defecto) deja en
  `kh_com.log` una línea `stutter:` con su etapa y sus costes; las cargas HD
  se separan en cabecera, decodificación y subida, con la carga más lenta.
- `profile_hd_resources.py` guarda las 12 peores en `report.json` (`stutters`).

Es un diagnóstico, no una mejora de velocidad. Sin `KHCOM_FRAMELOG` las sondas
nuevas no consultan el reloj ni acumulan contadores (`gPortStutter.on`). `logic_ms` va del
final del log del cuadro anterior al inicio de este ciclo: la lógica del juego
y lo que corra entre ciclos, sin el autoguardado ni el propio log. Las cargas
se suman por partes y se nombra la más lenta del cuadro, no la última.

Los costes de glifos aquí instrumentados corresponden al productor latino
`port_hdtext.c`; la ruta japonesa y el proveedor stb del guest de 64 bits no
quedan cubiertos por ese contador. Cero no significa coste de texto nulo en
esas rutas. La compilación de ambas arquitecturas no es una medición de ambas.

`test_stutter_probe_contract.py --cc CLANG` ejecuta los wrappers/macros reales
con reloj determinista: sin sondas no hay consultas nuevas; con sondas se
conservan totales, fallos y el recurso más caro, y el caché no se recuenta.
Comprueba también el límite de `logic_ms`. Dos controles negativos rechazan
consultas con la sonda apagada y la atribución anterior que incluía el logger.
No enlaza SDL ni acredita rendimiento o GPU.

Revisión de Codex (27/10) atendida: fixtures de `test_hd_sprite_budget`,
`test_hd_sprite_fallback`, `test_hd_menu_warmup` y `test_hd_reload_mask`
reciben los nombres nuevos (`PORT_STUTTER_*`, `gPortStutter`,
`LoadCounted`) sin quitar aserciones; las cuatro pasan, y también
`test_stock_card_frames`, `test_hud_options_warm`, `test_native_renderer_reset`,
`test_harness_key_chords`, `test_hd_menu_light_renderer`,
`test_hd_ui_japanese_queue`, `test_hud_canvas_preflight`, `test_log_history` y
`test_menu_visible_bottom_band`. `test_video_sync` falla igual en main limpio
(`SDL_getenv` sin declarar en su fixture): preexistente, ajeno a este PR.

## Primeras heridas (puerta de evento, Windows 32 EU, GPU, tiempo real)

Tres corridas `traverse-door` HD sí/no. Variación alta entre corridas
(18 s frente a 37 s para el mismo caso): el equipo tenía otra carga, así que
sólo cuenta lo que se repite.

| Cuadro | Repite | Coste | Etapa |
| --- | --- | --- | --- |
| 327 (entrar a las cartas de mapa) | 3/3 | 40–55 ms | 6 cargas HD (31 ms sumados: decodificación 11, subida 17,5) + 12 glifos nuevos (8,6 ms) |
| 293/294/402 | 1/3 | 90–125 ms | una carga HD de ~97 ms (banner, borde, carta): no confirmada, coincide con la corrida ruidosa |
| 468, 68–70 | 1/3 | 90–186 ms | `present` sin cargas: pendiente de aislar |
| Original 792–799 | 1/3 | 27–50 ms | PPU 22–43 ms con HD apagado |

## Siguientes pasos (sin aplicar todavía)

1. Cuadro 327: la copia blanca de cada sprite HD duplica la subida; crearla
   sólo cuando un fundido la pida, o precargar las cartas del menú durante
   la transición. Medir antes/después con la misma fixture.
2. Glifos: precalentar dígitos y letras frecuentes al crear cada fuente.
3. Repetir en un equipo sin carga externa para confirmar o descartar las
   cargas de ~97 ms y los `present` largos.

## Paso 2: glifos precalentados (tarea `optiport-v22-pacing`, 27/10)

La vista profunda mostró que los 12 glifos del cuadro 327 son de la fuente de
menús (estilo 0) a la escala de la capa de pantalla (6,75), no a la del mundo.
`PortHdTextWarmGame` (port_hdtext_warmup.inc, todos los backends) prepara antes
de jugar el juego latino del estilo 0 a las escalas de mundo y menú (sus dos
ranuras) y los dígitos de celda del estilo 2; una escala posterior sigue
renderizando al usarse.

| Corrida (puerta, W32 EU, GPU) | Glifos en 327 | Cuadro 327 | Preparación al arrancar |
| --- | --- | --- | --- |
| Antes, 3 corridas | 12 (4,3–8,6 ms) | 38–55 ms | — |
| Después 1 | 0 | 34,9 ms | 174 ms |
| Después 2 | 0 | 109 ms (subida 23 ms, present 43 ms) | 173 ms |
| Después 3 | 0 | 115 ms; cuadro 326 con PPU 462 ms sin cargas | 192 ms |

Las corridas 2 y 3 coinciden con carga externa del equipo: cuentan para
«glifos 0», no para el tiempo total. El coste se mueve a una sola vez antes del
título (~0,17 s). Queda en 327 la parte de las 6 cargas HD (decode ~10 ms,
subida ~12 ms): siguiente paso, la copia blanca bajo demanda o la precarga.
