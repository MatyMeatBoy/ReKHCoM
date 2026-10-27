# OptiPort 2.2: vista profunda de tirones

Tarea `optiport-v22-profiling`, owner `claude-v22`, 27/10/2026. Pedido del
usuario: «una vista profunda a las heridas donde haya stutters».

## Qué añade

Con `KHCOM_FRAMELOG` (el perfilador ya lo activa):

- `frames.csv` suma columnas por cuadro: `logic_ms` (lógica GBA desde el
  ciclo anterior), `ppu_ms`, `present_ms` (capas HD incluidas) y los costes
  puntuales de ese cuadro: `hd_loads`/`hd_load_ms` (decodificación y subida
  de sprites HD) y `glyphs`/`glyph_ms` (glifos HD renderizados por primera vez).
- Cada cuadro de más de `KHCOM_STUTTER` ms (25 por defecto) deja en
  `kh_com.log` una línea `stutter:` con su etapa y sus costes; las cargas HD
  se separan en cabecera, decodificación y subida, con el último sprite.
- `profile_hd_resources.py` guarda las 12 peores en `report.json` (`stutters`).

Es un diagnóstico, no una mejora de velocidad. Sin `KHCOM_FRAMELOG` no se
lee ningún reloj ni se cuenta nada (`gPortStutter.on`). `logic_ms` va del
final del log del cuadro anterior al inicio de este ciclo: la lógica del juego
y lo que corra entre ciclos, sin el autoguardado ni el propio log. Las cargas
se suman por partes y se nombra la más lenta del cuadro, no la última.

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
