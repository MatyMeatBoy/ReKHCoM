# Linux: terminal, launcher y Steam Deck

Consultar cuando el mismo paquete funciona desde terminal/escritorio pero falla
desde otro launcher o Gaming Mode. Este módulo organiza el diagnóstico; no
demuestra la causa de un reporte nuevo ni certifica compatibilidad de hardware.

## Localizar la primera diferencia

Identificar paquete publicado y hashes, arquitectura y ejecutable realmente
lanzado. Leer sus scripts, no sólo el tag o el main actuales: el empaquetado
puede haber cambiado. Distinguir Linux nativo (con o sin Steam Linux Runtime)
de Windows con Proton. No cambiar Proton para un ELF nativo sin definir
qué se está probando. Gaming Mode y el servicio `gamemoded` son cosas distintas.

| Última fase observable | Comparación mínima que decide el paso siguiente |
| --- | --- |
| El acceso directo no aparece | Cuenta/registro del cliente y entrada añadida; no diagnosticar el motor antes de que sea lanzado. |
| Aparece, pero no llega al wrapper o ejecutable directo | Target, Start In, argumentos efectivos, permisos/intérprete y ruta tras mover o actualizar. Un log sin cambios sólo es indicio si se comprobó que esa fase debía escribirlo y podía hacerlo. |
| Wrapper entra, hijo no inicia | Código de salida y error del loader, ELF/ABI e intérprete dinámico; dependencias incluidas frente a runtime seleccionado. |
| El proceso sigue vivo, sin imagen útil | Error SDL/EGL/Vulkan, backend y dimensiones reales, ventanas creadas, ventana visible y foco; distinguir crash, ventana oculta y proceso bloqueado. |
| Imagen correcta, controles fallan | Eventos que recibe la aplicación y ventana destinataria, Steam Input y acciones reservadas por Steam; no suponer que toda tecla llega al juego. |

Comparar terminal directa, Steam en escritorio y Gaming Mode con el mismo
paquete y configuración. Registrar comando/argv, cwd, salida, PID y última
fase alcanzada. Comparar sólo variables pertinentes como `PATH`,
`LD_LIBRARY_PATH`, `LD_PRELOAD`, `DISPLAY`, `WAYLAND_DISPLAY`, `XDG_RUNTIME_DIR`
y overrides SDL/Vulkan, omitiendo valores privados; no volcar todo el entorno.
Cambiar una condición por vez y conservar el resultado anterior.

Separar proceso creado, renderer creado, marcador previo a la llamada al
juego, entrada efectiva y primera presentación. Un marcador anterior al guest
sólo prueba que se alcanzó su escritura: no acredita ejecución del guest ni
GPU lista. Una prueba posterior puede crear una textura y configurar blend;
un fallo después del marcador todavía puede pertenecer al driver/renderer.
Registrar inicio, resultado/error y fin en la frontera que discrimine esas
hipótesis, sin convertir la última línea del log en causa demostrada.

Si 32 bits funciona y 64 falla, comparar los caminos efectivos de renderer,
fuentes, heap y assets además de ABI y argumentos consumidos; reenviarlos desde
el shell no basta si el siguiente parser los ignora. Leer la política de retry
del paquete exacto: si Automático debería recuperar en 32 ante un error normal
previo a entrada, el fallo de ambos modos requiere comprobar si hubo retry,
excepción, bloqueo o error posterior. No inferir la causa por arquitectura ni
aplicar retroactivamente una política añadida después del release.

Una `.desktop` usa reglas propias para `Exec`, incluidos escapes de cadenas,
argumentos y porcentajes; no equivale a pegar una línea de shell ni a los campos
de Steam. Probar el consumidor real del acceso directo, además de ejecutar el
script. Un fixture útil incluye espacios, porcentajes y directorio inicial
ajeno. No asumir que `Documents` es inválido ni exigir una carpeta `Games`.

Capturar stderr y fallos tempranos aunque no exista terminal. Si el logger
escribe junto al ejecutable, comprobar permisos y el comportamiento cuando no
puede escribir antes de usar ausencia de log como prueba. No vaciar
`LD_PRELOAD`/rutas de bibliotecas o forzar un backend como reparación general:
son experimentos acotados cuando el error los justifica, con efectos sobre
overlays, mandos o carga de dependencias que también deben comprobarse.

Una VM Linux o gamescope anidado puede probar partes de esta cadena; no replica
por sí sola la sesión, GPU, Steam Input ni cliente de un Deck físico. Separar
fixture de quoting, inicio del loader, primera presentación y confirmación real
del reportero. Conservar progreso y ajustes entre pruebas.

## Casos primarios leídos el 27/10/2026

Estos casos aportan mecanismos y límites, no recetas para todos los ports.
No se ejecutó su código ni se copió una implementación externa.

| Fuente | Lo documentado y cómo usarlo |
| --- | --- |
| [OpenGOAL, informe Q2 2025](https://opengoal.dev/blog/progress-report-q2-2025/) | Describe versiones del launcher AppImage con empaquetado defectuoso y error `EGL_BAD_PARAMETER`. Separar launcher y juego, y buscar el error concreto; no atribuir cualquier pantalla negra a EGL. |
| [OpenGOAL, instalación y acceso directo](https://www.opengoal.dev/docs/usage/installation/) | El ejecutable del juego vive en una carpeta de versión; el acceso directo requiere actualizarse al cambiarla. Verificar el destino efectivo después de mover/actualizar, sin modificar datos del cliente a ciegas. |
| [Zelda64Recomp #184](https://github.com/Zelda64Recomp/Zelda64Recomp/issues/184) | El juego abría en escritorio, pero el acceso ni aparecía en Gaming Mode. La conversación apunta al registro del cliente. Es un síntoma distinto de un acceso visible que inicia y se cierra; no promover el workaround comunitario de copiar `shortcuts.vdf` a regla general. |
| [Zelda WWHD Recomp #112](https://github.com/ZeldaWWHDRecomp/ZeldaWWHDRecomp/issues/112) | El diagnóstico con logs encontró el menú dibujado en la ventana TV mientras Gaming Mode mostraba GamePad. El reportero confirmó la corrección posterior `7aa1ec8`. Verificar destino del dibujo y foco; recibir una tecla no demuestra que la UI esté visible. |
| [Valve steam-for-linux #7219](https://github.com/ValveSoftware/steam-for-linux/issues/7219) | Reporte histórico de 2020 sobre truncado de Target/Start In con espacios. Justifica una regresión de rutas; su cierre no prueba presencia ni ausencia de ese fallo en el cliente actual. |
| [ReKHCoM, host publicado](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/windows64/windows64_host.c#L558-L570) y [probe GPU](https://github.com/MatyMeatBoy/ReKHCoM/blob/8c613ae5d/port/ps5/shared/host_gpu_targets.h#L18-L33) | El marcador se escribe antes de llamar al guest; éste puede solicitar después una prueba real de textura/blend. Ilustra por qué entrada, capacidad gráfica y presentación requieren evidencia separada; no identifica la causa de un reporte. |
| [Desktop Entry, `Exec`](https://specifications.freedesktop.org/desktop-entry/latest/exec-variables.html) | Especifica quoting y dos capas de escapes; `%` literal es `%%`. Validar el parser usado, no sólo la salida textual del generador. |
| [Valve gamescope, README fijado](https://github.com/ValveSoftware/gamescope/blob/d175688ad4ec490dc7b0695c5c62f8807684a1f1/README.md) | Documenta sesiones embebidas/anidadas, Xwayland y pantalla virtual. Una ventana de escritorio o una resolución solicitada no prueban la superficie que finalmente presenta el compositor. |

Guardar una nueva lección sólo con trigger, mecanismo, comprobación y límites.
Si falta el log/dispositivo, conservar la hipótesis en el proyecto. No presentar
una revisión de fuentes como un bug propio resuelto o una ganancia de velocidad.
