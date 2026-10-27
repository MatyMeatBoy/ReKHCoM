# Plan serial ejecutable de cierre OptiPort — 27/10/2026

Carril `optiport-serial-profile-contract`, owner `codex-profile-contract`.
Preparado sin ejecutar juegos, builds ni benchmarks. No altera335/342/345.
Los cuatro bloques parciales siguen parciales hasta obtener/revisar evidencia.
10/14≈71% sólo cuenta la batería original; no todo2.1 ni estabilidad.

## Entradas y separación de evidencia

Usar fuente/artifactos ya construidos del MISMO SHA, EU y mismos assets propios.
Elegir build32 con khcom.exe/SDL3.dll, build64 con su SDL3.dll, host rekhcom64.exe
y guest-eu.bin explícitos. ROM propia, sprites importados y assets comunes con
fonts/KHMenu.otf. Nunca la instalación principal ni datos/partidas originales;
los runners crean carpetas privadas y sólo ejecutan copias. No descargar ni
reimportar durante muestras. Conservar SHA256 del ZIP/artifacto además de
exe/DLL/guest/ROM y fingerprints de contenido assets/sprites que guarda el arnés.
Anotar CPU/GPU/driver/OS/resolución/backend y procesos concurrentes. No builds,
VMAndroid, recodificación ni otros benchmarks a la vez. Audio dummy siempre;
una medición con dummy no certifica altavoces/latencia de audio físico.

Generator no ejecuta procesos del juego:

```powershell
python port/tools/profile_serial_plan.py --build32 C:/ruta/build32 --build64 C:/ruta/build64 --exe64 C:/ruta/build64/rekhcom64.exe --guest C:/ruta/build64/guest-eu.bin --rom C:/ruta/propia/B8CP.gba --sprites C:/ruta/propia/hd/sprites --assets C:/ruta/build32/assets --out build/serial-v21-plan
```

Revisar serial-plan.json y, sólo cuando root libere la fase prerelease:

```powershell
& ./build/serial-v21-plan/run-serial.ps1
```

Ocho invocaciones seriales:3repeticiones de32/64 alternando orden AB/BA/AB,
luego2corridas separadas de calidad. Se detiene al primer fallo y conserva todo.
Cada invocación hace original/HD con los mismos workloads,1800frames,
calentamiento hasta300 y ≥120muestras warm. Estos números son duración/muestreo,
NO presupuesto de rendimiento ni objetivo prometido. No pasar --fast para
pacing. --software es una comparación CPU aparte, no GPU del usuario.

## Fase1: baseline válido y comparable

`profile_hd_resources.py` conserva título idle sin teclas (no entrar al modal),
dos entradas de debugbattle30/31 escenario5 y puerta de evento de Traverse.
Las etiquetas históricas monstro-battle NO prueban el estado de historia del
reportero: son enemy/background de debug. Cada carga guarda el fixture pedido,
trace de modo y dos snapshots read-only `progress` con flags/floor/WorldId/room/
story/level. La puerta exige WorldId10 y pose real; no sustituirla por castillo.

El opcional --scenarios monstro-opening ahora prepara
room:10303→eventplay74, exige modoEvent y WorldId5. Índice MAP3 corresponde a
Monstruo5; índice5 es Halloween6. Mapa leído EU comunicado por coordinador:
[1,7,2,5,4,6,3,8,9,10,11,12,0]. No usar índice como WorldId ni afirmar SRAM/
opciones del reportero equivalentes sólo porque evento74 arrancó. USA y la
SRAM real se mantienen como repro funcional separado con estado efectivo.

Cold significa primeros frames del PROCESO, no caché fría del SO/disco ni
primer upload completo. Warm significa intervalo explícito del reporte; no
mezclarlo con cold. CSV completo/ordenado/sin huecos, floats finitos, muestra
suficiente y modo real son requisitos. Pacing normal exige pasos de lógica[1].
Reportar p50/p95/p99/mean de trabajo outer-VBlank e intervalos y cold por
separado; percentil lower-order floor((n-1)*q), tamaños de muestra registrados.
No combinar todas las repeticiones borrando sus diferencias.

Trabajo outer-VBlank incluye event pump y esperas modales; NO es tiempo de
cada cuadro del menú ni decode/playback de película HD. Present/vsync puede
bloquear; tampoco equiparar tiempo CPU a GPU. No usar este CSV para cerrar
latencia de previews o media sólo porque el proceso salió0.

## Fase2: calidad32/64, fuera de la muestra temporal

Corridas --quality-capture tienen timing_comparable=false: BMP/readback/E/S
pueden contaminar tiempos. Captura final obligatoria; si no existe, falla la
postcondición, no se declara calidad verde. Comparar misma resolución, vista,
estado/logic frame, texto/subtítulos, HUD/cartas/targets y assets fingerprint.
Conservar BMP y SHA. Hash distinto de BMP no demuestra calidad distinta por
sí solo (GPU, fases de animación); revisar imagen real y geometría/completitud.
La falta de readback guest requiere captura host equivalente antes de cierre.

## Fase3: galería/asíncrono con los recursos existentes

Probar GBA, HD y ReKHCoM híbrido;32 y64 en copias equivalentes. Fuentes propias:
posterGBA existente + fondoROM, capturaHD sidecar y película autorizada corta.
Cold: carpeta privada sin sidecar generado; warm: volver a A tras B/C dentro
del mismo menú y reabrir. No borrar sidecars/PNG del import original. Mismo
evento, layout, idioma y calidad; luego cambio EN→JP de335 para invalidación.

Rutas funcionales32 existentes (no puntúan pacing porque usan FAST/dummy):

```powershell
python port/tools/test_scene_gallery.py --exe C:/ruta/build32/khcom.exe --rom C:/ruta/propia/B8CP.gba --work build/gallery-functional --kind gba
python port/tools/test_scene_gallery.py --exe C:/ruta/build32/khcom.exe --rom C:/ruta/propia/B8CP.gba --work build/gallery-field --kind gba --from-game
python port/tools/test_scene_gallery.py --exe C:/ruta/build32/khcom.exe --rom C:/ruta/propia/B8CP.gba --work build/gallery-hd --kind hd --hd-clip C:/ruta/propia/clip-corto.mp4 --pause
```

No dar a esos runners el host64 renombrado: todavía no copian guest ni pasan
su CLI. Para64 replicar las MISMAS variables del runner con comando
`rekhcom64.exe guest-eu.bin --data CARPETA_PRIVADA --rom ROM --language es`,
assets/HD copiados allí y KHCOM_HOST_HIDDEN=1. Esto es un gate pendiente de
adaptación/ejecución del integrador, no un runner64 ya verificado por este PR.

Para latencia/GPU: driver real, FAST AUSENTE,45selecciones deterministas (15
por tab) y captura de cuadros de entrada→primer respaldo→HD listo, con tiempo
real de presentación del vídeo. Anotar muestras en CSV request_ms/first_ms/
ready_ms/source/event/runtime/outcome, p50/p95/p99 por tab y cold/warm. Si sólo
hay captura60Hz, declarar cuantización~16.7ms, no precisión submilisegundo.
No hay timestamps request/ready suficientes hoy en frames.csv: no inventarlos.

Exigir respuesta de entrada mientras decoder tarda, un worker, serial viejo
rechazado, cacheGBA≤1MiB/3entradas, imagen del mismo evento y sin borde gris
exterior. Corrupto/ausente mantiene GBA, cancel no espera decoder, ningún
historial/SRAM/achievements cambia. Registrar hashes de memoria/archivos con
KHCOM_GALLERY_AUDIT=1 y vídeo normal/skip desde título/campo. PR339/342 tienen
escenarios y pruebas de productor; su PASS no sustituye estas capturas.

## Fase4: submenús, música y vídeo

Submenús: Deck/MapInspect/Status/Jiminy y Mogurientrada/shop/charge. Repetir
ruta igual cold y warm, mantener entrada/cambio pestaña/retorno. Existing
`test_gameplay_menus.py --build BUILD32 --version eu --cycles 3` separa cada
ruta; confirmar modo de entrada y retorno posterior. `test_menu_transition_profiles.py`
prueba clasificación, no latencia. Para render serial activar
KHCOM_HD_MENU_PROFILE=1 y conservar cada `hd-menu-perf` (draw+present, no GPU
puro), separar primer dibujo de estable y excluir capturas. El modal comparte
framecounter: no convertir sus líneas en avances de lógica ni sacar p95 de
outer-VBlank.64 necesita mismo adaptador explícito de host/guest, no falsa32.

Música: dos tracks locales hablados/instrumentales según escenario, normal,
loop corto, fin/vueltaGBA y toggles; copiar sólo tracks/loop requeridos a hd/music
privado. Exigir log `hd music: song N -> nombre`, no sólo opción hd_music=1.
Medir entrada/cola real con evidencia de underrun/backpressure y CPU/RSS; dummy
valida el camino de software, dispositivo físico se registra aparte.
`test_hd_music_failures.py` y `test_hd_music_loader.py` cubren cero progreso,
rechazo de cola, cancel/relevo y bounds: son tests de productor, no música real
ni p95. Ejecutarlos seriales antes del soak cuando ya no compita prerelease.

Vídeo: opening, evento74 contextual y finales seek deSora/Riku con source,
audio/subs iguales. `test_hd_scenes.py --hd COPIA_PRIVADA --build BUILD32 --jobs 1
--frames 900 --only NOMBRE_EVENTO` acota mapa y exige apertura/playback; su stop
de segundos es prueba funcional, no reproducción completa ni AV-sync. Revisar
CLI nombres antes de ejecutar.64 usa host/guest y mismo MP4, no cambiar encode
ni resolución para obtener mejor cifra. Capturar frames decoded/dropped,
primera imagen, skip, retorno, audio/subtítulos y RAM; fallido nunca salta historia.
No atribuir el trabajo bloqueante del loopHD a intervalos de gameplay.

## Fase5: resistencia dirigida, no campaña completa

Una vez fases1–4 dan evidencia interpretable, repetir patrón que cubra riesgos:
20ciclos de cada submenú;10selección/replay/skip por tab; loops de música largos
y cortos; device/reset/pérdida foco; guardar/recargar manual32↔64 y namespace
quicksaves. Cantidades son propuesta de muestreo dirigida, no SLA/budget.
Mantener fixture SRAM único que encontró puertaNULL y fixtures sintéticos
corruptos/ausentes. No exigir campaña completa ni llamar al catálogo GameFAQs
un corpus descargado. Tomar RSS/private al inicio/pico/final y transitorios de
cache con la misma resolución; investigar crecimiento retenido por ciclo sin
imponer umbral inventado. Repetir fallo con evidencia antes de atribuir a RAM.

Cerrar cada bloque sólo con SHA/artifact/hash, fixture efectivo, opciones,
backend, repeticiones, distribuciones, calidad equivalente y riesgos resueltos
o límites explícitos. RSS/working set y PrivateUsage(commitWindows) son proceso,
no VRAM; Linux /proc/status no aporta private en este arnés y queda null. No
transformar compilar, salir0 o PRmerge en confirmación física o estabilidad100%.

## Defectos concretos corregidos en este PR

Reporte antiguo aceptaba muestras vacías/truncadas/NaN, VmData como private y
no copiaba assets comunes a la carpeta aislada. Ahora falla cobertura/mode/
quality y copia assets una vez porinvocación con hash, admite host64+guest
explícitos, guarda p50/p95/p99 y snapshots efectivos. Capturas opt-in separadas.
Siete tests puramente lógicos pasan; ninguna cifra de rendimiento nueva.
La evidencia600frames249c55e se conserva como observación histórica con sus
límites; no se recalcula ni se vuelve válida de forma retroactiva.
