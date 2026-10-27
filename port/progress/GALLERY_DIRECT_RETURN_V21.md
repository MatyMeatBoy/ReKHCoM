# Galería: retorno directo — 27/10/2026

Tarea `gallery-direct-return-no-flash`, owner `codex-gallery-direct-return`.
Base4452d0a23; trabajo separado de la prerelease e71,327/335 y337.

## Causa y cambio

El productor nativo `mode_event.c`/`mode_movie.c` solicita
`PortSceneGalleryReturn`; `PortSceneGalleryFrame` restaura el checkpoint,
reinicia audio, termina replay y llama `PortUiGalleryResume` antes de
`VBlankCycle`. El camino HD síncrono de `SceneMenuInput` termina su loop,
restaura canvas/mixer y llama al mismo resume. Los caminos de fallo también
vuelven por ese punto común.

Resume usaba `PortUiOpenOptions`, que arranca `sFade=0`. El consumidor
`PortUiMenuCanvas` permanece falso hasta8cuadros y `PortUiSceneFade`
aplica oscurecimiento al título/campo restaurado. No era un salto requerido
por la historia: era reutilización de la entrada genérica del menú.

Se conserva toda inicialización del menú y selección/scroll/tab/rotos, pero
resume empieza con el lienzo de lista completamente visible (`sFade=16`),
sin cierre/retorno pendientes. Se suprime entrada hasta soltar para que el
skip/confirm sostenido no inicie otro replay. La apertura/cierre normal de
Opciones conserva sus fades. No añadir retraso, fade largo ni avance GBA.
No tocar los mecanismos existentes de checkpoint, SRAM, mezcla o renderer.

## Evidencia y límites

`test_gallery_direct_return.py` compila las funciones reales de resume y los
dos consumidores de presentación. Antes: assertion RED primer lienzo no era
menú. Después: GREEN primer lienzo galería sin fade de escena, elección/scroll/
tab/rotos conservados, input suprimido y captura offline sin abrir menú.
Los orígenes title/field y normal/skip comparten esta función; el harness no
emula sus juegos ni acredita píxeles GPU, audio, dispositivo o duración real.
Syntax-only Windows UI y diffcheck pasan. No se ejecutó juego/build pesado.

Las plataformas con secciones de juego (Windows/Linux/Android/guests actuales)
restauran snapshot. El fallback histórico sin secciones vuelve al título para
reinicializar antes del resume: su etapa de reinicialización no está cubierta
por este arreglo ni se declara libre de cuadros intermedios sin QA específica.

## Escenarios seriales para el coordinador — no ejecutados aquí

Copia aislada, audio dummy, ROM propia; nunca instalación principal. Mantener
hash de ejecutable/config/SRAM. Ejecutar título y campo guardado × normal y
skip × tabs GBA y ReKHCoM; HD puro adicional. Fuente válida y ausente/corrupta
para comprobar fallback. Todos los casos deben retener elección, tab, scroll,
config, SRAM, progreso/achievements; no saltar historia ni filtrar botón final.

Arnés existente `test_scene_gallery.py --exe ... --rom ... --work ... --kind
gba|hd|hd-error [--from-game] [--save ...] [--hd-clip ...]` conserva aislamiento
y auditoría. Es una prueba funcional, no una verificación de cada cuadro GPU.
Para captura usar driver GPU real oculto y grabación externa/`KHCOM_RECORD`
con ticks de menú habilitados según harness. La galería incrementa su tick
sólo al dibujar la lista: `KHCOM_GALLERY_KEYS=30:120,80:-1`,
`KHCOM_GALLERY_SHOTS=29,30,31,32,33` da referencias antes y después de replay
(tick31 primer dibujo después, no prueba por sí solo de la ausencia de frames
extra). Revisar grabación frame a frame entre última imagen de escena y
primer lienzo de lista, no sólo las capturas31..33. Para normal usar escena
corta/clip hablado propio; para skip probar ambos niveles de confirmación.
Registrar `KHCOM_GALLERY_AUDIT=1`; exigir game/map/config/files/recom/rank=1.
Además rendererreset y pérdida de foco durante pausa, sin atribuir cobertura
al test de helpers.

## Investigación de previews — siguiente carril, sin implementar aquí

Tabs reales: GBA, HD, ReKHCoM híbrido. La frase del usuario «ambos modos»
requiere contrastar presentación GBA/ReKHCoM y no eliminar HD puro.
Existe un buffer CPU y textura seleccionada, worker HD único con debounce75ms,
sidecar poster y descarte serial; GBA usa poster PNG o fondo ROM inmediatamente.
Cambiar selección pierde el buffer anterior; el decode PNG se hace sincrónico,
y no hay caché LRU de las últimas selecciones. En guest, sin worker hosted,
la disponibilidad depende del sidecar. El recuadro del menú ya rellena blanco;
los bordes grises pueden estar en el poster/fondo/ratio: capturar antes de
suponer un color concreto. La resolución320/640 y reescalado entero merecen
medición de entrada/p95/memoria antes de ampliar caché.

Siguiente pieza debe reclamar `optiport-previews-both-modes-perf` y/o
`previews-both-modes-visual-fallback`, coordinar335 para claves de idioma y
generación, reutilizar fondos de ROM/UI existentes para fallback legible y
preservar el GBA como respaldo inmediato mientras el HD tarda. No arte externo.
