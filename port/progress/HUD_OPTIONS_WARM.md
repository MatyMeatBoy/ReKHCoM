# HUD al volver de Opciones

Claim `hud-toggle-battle-warm`, owner `device_profiles_sol`.

## Mecanismo y alcance

`Change(ITEM_HDHUD)` cambia `hdHud` mientras el juego está pausado.
`Present` ya prepara el pilot HP/label durante Opciones. Sin embargo,
`PortHdHudSpriteRegister` rechaza `PortUiActive()`, y el canvas de Opciones
omite `PortPpuRenderHud`. Las piezas content-keyed se decodificaban/subían
al primer HUD después de reanudar.

El último cuadro de cierre ya está negro. Después de `PortUiDraw` desactivar
la UI, pero antes de devolver control a la lógica, `PrepareHudAfterOptions`
recorre sólo el HUD/OAM congelado mediante `PortPpuRenderHud`, en el mismo
hilo del renderer. No recorre el pack ni adelanta lógica, no espera timers,
no crea tareas/cargas pendientes para cancelar. Desactivar HUD o cerrar fuera
de un HUD activo no prepara nada. Volver a abrir conserva caché normal.
El buffer HUD y las cachés existentes mantienen sus presupuestos y ownership.
Los archivos ausentes/uploads fallidos conservan fallback GBA y miss cacheado.

## Evidencia CPU

`python port/tools/test_hud_options_warm.py` compila las funciones reales
`PortUiDraw`, `PortUiActive`, `PortHdHudSpriteRegister`, `LoadHudAsset` y el
helper/callsite de `Present`; toma también la sentencia real del toggle.
El recorrido OAM es un stub que entrega una pieza RGBA al registro real;
no demuestra el conjunto de piezas de una batalla ni tiempos GPU.
Antes: exit77, falla `decodes==1 && creates==2` al finalizar cierre (0/0).
Después: PASS 1 decode/2 uploads antes de reanudar, 0 adicionales al primer
render siguiente y reapertura; cierre aún animando no carga; controles
missing/upload-failure/off/non-HUD conservan fallback sin IO repetido.
`test_hd_hud_invalidation.py` PASS y `test_options_route.py` 3/3 PASS.
La regresi�n OOM extrae `GrowPixelBuffer` real y mantiene `SDL_realloc`
fallando: la versi�n inicial del helper termin� en PortFatal (exit88).
Con la guarda de capacidad, cero llamadas al allocator/fatal, se cierra UI y
se preservan puntero, capacidad y contenido previo; buffer ausente tambi�n
omite preparaci�n. Esto NO resuelve el OOM del render normal: ese consumidor
hist�rico a�n exige crecimiento fatal; `hdHud=0` no evita su canvas HUD.
Requiere preflight transaccional antes de suprimir el HUD nativo en PPU y
fallback de composici�n separado. No afirmar recuperaci�n de OOM esencial.
Syntax-only x32 freestanding de `port_main.c` PASS. No build completo ni juego.

## Fixture GPU para coordinador (pendiente)

Usar exclusivamente copia aislada del nuevo ejecutable, SDL3.dll, ROM privada
propia y los assets aprobados. No ejecutar/escribir instalación Quiver.
Obtener el ID vigente (no fijar números del enum):

```python
import sys
sys.path.insert(0, 'port/tools')
from test_text_options import item_numbers
print(item_numbers()['HDHUD'])
```

En cada directorio de caso nuevo: config `hd_hud=0`, `hd_cards=1`,
`view_mode=4`, ventana1920x1080; assets core del mismo build. Entorno:
`SDL_VIDEO_DRIVER=windows`, `SDL_AUDIO_DRIVER=dummy`,
`SDL_RENDER_DRIVER=direct3d11`, `KHCOM_BATTLE=204:8:0`,
`KHCOM_KEYS=60-63:A`, `KHCOM_OPTIONS_TOGGLE=180`,
`KHCOM_OPTIONS_ACTIONS=10:<ID_HDHUD>:1`, `KHCOM_OPTIONS_HOLD=30`,
`KHCOM_VPAD=0-0:SOUTH` (el harness cuenta cuadros del menú),
`KHCOM_OPTIONS_AUDIT=1`, `KHCOM_HD_MENU_PROFILE=1`,
`KHCOM_HD_SPRITES=<sprites aprobados>`, `KHCOM_WINDOW=1920x1080`,
`KHCOM_OUTPUT_SHOT=175,225,226,227,240,260`, `KHCOM_QUIT=265`.
El proceso recibe `--rom <ROM> --data-dir <caso> --language es`.
Ajustar capturas al tick de cierre registrado si cambia la ruta.

Comparar SHA anterior/nuevo con misma entrada fría y medir cuadros en torno
al cierre/primer HUD, no sólo FPS medio. La línea `HD HUD: prepared current
HUD before gameplay in ... ms` mide el recorrido de preparación; las líneas
`HD HUD: cached sprite ...` deben precederla, sin nuevas cargas de esas piezas
al reanudar. Comparar píxeles finales y fallback, no sólo logs.
Repetir caliente con `hd_hud=1`, acciones10:ID:-1,12:ID:1; y un caso parcial
omitiendo sólo una pieza identificada en los logs de la copia de prueba.
Registrar renderer efectivo, SHA, config, percentiles y límites. No se afirma
mejora medida de latencia ni equivalencia GPU con la evidencia CPU.

## Banner español: import desfasado

`gWorldselectTitleSpanishBanner` exige canvas1112x128, aspecto8.6875: el aviso
960x128 vs8.688 concuerda con PNG antiguo. No cambiar la receta válida para
aceptarlo. La reparación de sprites por huella/proporción existente y su guard
Android (#329) siguen siendo el carril pertinente; reimportar en copia aislada
y comprobar cero stale es la siguiente evidencia, no un nuevo arte.
