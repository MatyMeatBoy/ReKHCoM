# Aprendizajes de ShiningSource 0.8.8 para Android y online

Fecha: 2026-11-16. Seguimiento documental de [#647](https://github.com/MatyMeatBoy/ReKHCoM/issues/647).
No cambia el runtime, el protocolo ni los paquetes de ReKHCoM.

ReKHCoM aportó el diseño de imagen relocatable y fallback de memoria ejecutable
que informó la actualización Android de ShiningSource. Este retorno propone
casos de regresión y revisiones concretas; no presenta esas técnicas como nuevas
ni atribuye a ReKHCoM todos los defectos encontrados en otro juego.

## Identidad de las fuentes

- ReKHCoM examinado: [`6c1951273fc2ccf79369cead1ba207e7297b931d`](https://github.com/MatyMeatBoy/ReKHCoM/tree/6c1951273fc2ccf79369cead1ba207e7297b931d).
- ShiningSource publicado: [`3433811a8e429ceb67009b20dd3d9915fe3f74e3`](https://github.com/MatyMeatBoy/ShiningSource/commit/3433811a8e429ceb67009b20dd3d9915fe3f74e3), destino del tag `v0.8.8`.
- [Informe técnico completo](https://github.com/MatyMeatBoy/ShiningSource/blob/3433811a8e429ceb67009b20dd3d9915fe3f74e3/docs/REKHCOM_LESSONS_2026-11-16.md),
  [registro de verificación](https://github.com/MatyMeatBoy/ShiningSource/blob/3433811a8e429ceb67009b20dd3d9915fe3f74e3/docs/ANDROID_0.8.8_VERIFICATION.md)
  y [release con artefactos/checksums](https://github.com/MatyMeatBoy/ShiningSource/releases/tag/v0.8.8).

Las observaciones de código siguientes corresponden a esas revisiones. Los
resultados ShiningSource proceden de su registro; este trabajo documental no
ejecutó ReKHCoM ni revalidó sus backends.

## Evidencia online: tres alcances distintos

| Evidencia | Qué permite afirmar | Qué queda sin confirmar |
| --- | --- | --- |
| Confirmación nueva del usuario: pudo conectar Android y Windows con ShiningSource 0.8.8 | Existe una conexión real entre esas dos plataformas informada por el usuario | Modelos, ABI, versión Android, duración, secuencia jugada y métricas de sincronización no fueron suministrados |
| Prueba automática Android x32 relocatable frente a Windows: 24000 frames por cliente, 23509 labels comunes y cero diferencias del bloque registrado | La identidad corregida admite esa pareja y los bloques/status coinciden por label en esa secuencia | No es prueba ARM/Windows, comparación completa de memoria ni cobertura de todas las recuperaciones |
| Replays locales ARM32 y ARM64: 32000 hashes de frame iguales, audio exacto | Equivalencia de la secuencia local canónica entre esos backends, incluida la corrección ABI | No es una sesión online ARM32/ARM64/Windows ni una prueba de latencia real |

La confirmación nueva complementa las pruebas automáticas; no amplía sus
mediciones retrospectivamente. No identifica el dispositivo del usuario como
Galaxy A11 ni como el teléfono reportado por RobNick. La instalación/ejecución
en esos teléfonos físicos continúa pendiente de confirmación identificada.

Hay además un reporte nuevo de **golpes remotos que no se oyen** y una solicitud
de un toggle experimental activado por defecto. Se está investigando en
ShiningSource: no está resuelto ni verificado y no se presenta aquí como una
mejora demostrada, una función publicada de 0.8.8 o un defecto de ReKHCoM.

## Revisiones transferibles, con pruebas de aceptación

### Lifecycle SDL3 y configuración real del sistema

El [host Android64](https://github.com/MatyMeatBoy/ReKHCoM/blob/6c1951273fc2ccf79369cead1ba207e7297b931d/port/android64/host/android64_host.c#L481)
examinado cambia `sBackground` en el polling y lo expone mediante `H_app_state`.
No registra un `SDL_AddEventWatch` en ese archivo. En el SDL3 usado por
ShiningSource, `SDL_SendAppEvent` llama a los watchers sin encolar los eventos de
app. Los casos correctos dentro de `SDL_PollEvent` no demostraban su recepción.

La adaptación ShiningSource usa un watcher, deduplica WILL/DID background y
espera `DID_ENTER_FOREGROUND` para restaurar pacing/audio después de la
restauración EGL. Antes de transferirla, comprobar semántica y hilo de entrega
del SDL fijado por ReKHCoM. Mantener la política de pausa propia de su juego/red.

Otro fallo observado fue la recreación SDL al inicializar MCC/MNC de la SIM en
API21. ShiningSource añadió `mcc|mnc` al juego; su launcher Java normal sigue
recreándose para actualizar idioma, fuentes y densidad. ReKHCoM tiene un
[contrato de configuración propio](ANDROID_LIFECYCLE_CONTRACT_20261028.md), con
actividades SDL y reglas que deben conservarse. No copiar la excepción de un
launcher Java normal a un launcher SDL.

**Aceptación:** render real, Home/retorno, input mantenido, densidad y cambio
real MCC/MNC; registrar las dos transiciones y verificar que sobreviva el mismo
PID. La prueba MCC/MNC debe releer la configuración después de inyectar y
restaurar valores en `finally`; API21 puede devolver shell exit 0 aunque Java
haya fallado. ShiningSource publicó
[fixture y runner](https://github.com/MatyMeatBoy/ShiningSource/blob/3433811a8e429ceb67009b20dd3d9915fe3f74e3/tools/tests/android_configuration_test.py).
Un `SDL_PushEvent` sintético no prueba el camino de entrega watcher-only.

### Identidad estable antes de parchear punteros

En ShiningSource, una identidad calculada sobre ROM con punteros nativos
parcheados separaba salas según arquitectura/dirección. Ahora captura los bytes
originales antes de `ss_patch_code_pointers`; el protocolo usa reglas 6.
Fuente: [codeptr.c](https://github.com/MatyMeatBoy/ShiningSource/blob/3433811a8e429ceb67009b20dd3d9915fe3f74e3/runtime/core/codeptr.c)
y [netlink.c](https://github.com/MatyMeatBoy/ShiningSource/blob/3433811a8e429ceb67009b20dd3d9915fe3f74e3/runtime/core/netlink.c).

No se atribuye ese defecto a ReKHCoM: su
[handshake incluye `NET_VERSION 5`](https://github.com/MatyMeatBoy/ReKHCoM/blob/6c1951273fc2ccf79369cead1ba207e7297b931d/src/sio/sio.c#L96)
y rechaza versiones incompatibles. La regresión transferible consiste en
comprobar que identidades de contenido/reglas/snapshot no dependan de punteros,
padding o direcciones de carga. Una huella de compatibilidad no es autenticación.

**Aceptación:** misma ROM y reglas, arquitecturas/direcciones distintas, imagen
forzada a moverse y ventana original ocupada; comparar negociación y bloques
por label. Separar esa prueba de determinismo completo y de experiencia jugada.

### Memoria adversa y fallback completo

ReKHCoM ya implementa
[reserva/relocation/W^X/memfd](https://github.com/MatyMeatBoy/ReKHCoM/blob/6c1951273fc2ccf79369cead1ba207e7297b931d/port/android64/host/android64_host.c#L634).
Los bordes a revisar son la búsqueda con alineación de 1 MiB y el único `write`
en `ExecFromMemfd`: un hueco suficiente alineado sólo a página, `EINTR` o una
escritura corta no deberían descartar una alternativa válida. Son propuestas
de robustez, no fallos reproducidos en un teléfono ReKHCoM.

**Aceptación:** ventanas históricas ocupadas; hueco exacto de 68 MiB y otro de
64 KiB; agotamiento con fallo limpio; rechazo/liberación de direcciones fuera
de 32 bits; movimiento y memfd forzados; metadatos de relocation malformados.
Los [tests ShiningSource](https://github.com/MatyMeatBoy/ShiningSource/tree/3433811a8e429ceb67009b20dd3d9915fe3f74e3/tools/tests)
incluyen fixtures de memoria y parser. Las páginas y permisos se validan contra
el tamaño real del sistema, sin hacer RWX ni tratar constantes GBA como fixups.

### Audio opcional y diagnósticos de resultado

El [inicio Android64 examinado](https://github.com/MatyMeatBoy/ReKHCoM/blob/6c1951273fc2ccf79369cead1ba207e7297b931d/port/android64/host/android64_host.c#L893)
inicializa vídeo/audio/gamepad conjuntamente. Si falla el subsistema de audio,
puede terminar antes de alcanzar la apertura tolerante del stream. Evaluar
audio separado y continuar sin sonido si la semántica del juego lo permite.
ShiningSource verificó 600 frames renderizados con un driver de audio inválido,
incluido ARMv7 por native bridge; no equivale a certificar todos los dispositivos.

ReKHCoM ya tiene journals e
[instrumentación de arranque](https://github.com/MatyMeatBoy/ReKHCoM/blob/6c1951273fc2ccf79369cead1ba207e7297b931d/port/android/app/src/main/java/com/rekhcom/android/StartupSdlActivity.java).
Sumar casos a ese sistema: arranque incompleto no demuestra un crash; una salida
normal no es error; los códigos internos normalizados deben coincidir en el
marker y la salida real. ShiningSource corrigió esa discrepancia en soft reset
75, sin afirmar que ReKHCoM use ese código. Compartir sólo diagnósticos acotados,
con versión/ABI y evidencia de la ejecución correspondiente, sin ROM/partidas.

### ABI ARM32 y trazas intermedias

Una declaración alias con argumento ancho y una definición `u16` hicieron que
ARM32 recibiera -48 en lugar de 65488. AAPCS permite confiar en la extensión
del caller. Hubo 90 frames de estado OBJ distinto pese a hash final/audio
iguales; normalizar ambos extremos produjo igualdad en los 32000 frames.
Incluir fuentes/declaraciones privadas del runtime y tipos `stdint`, no sólo
headers del juego. Asimismo, `--wrap` no intercepta una llamada ya inlineada
dentro del mismo archivo; un guard de otro backend puede filtrar direcciones
GBA válidas antes de traducirlas.

**Aceptación:** fixtures compilados/enlazados con el NDK real y trazas
intermedias. No deducir la ABI del sistema instalado sólo del modelo/SoC;
separar rechazo del instalador, loader, gráficos/audio y lógica del juego.
Son defectos demostrados en ShiningSource, no una imputación a ReKHCoM.

## Online: escenarios cooperativos para revisar un modelo VS distinto

ReKHCoM ya tiene ACK/retransmisión/revisión de slots, medición RTT, delay y
predicción adaptativos, rollback y detección de desync de batalla en
[sio.c](https://github.com/MatyMeatBoy/ReKHCoM/blob/6c1951273fc2ccf79369cead1ba207e7297b931d/src/sio/sio.c).
Su [transporte](https://github.com/MatyMeatBoy/ReKHCoM/blob/6c1951273fc2ccf79369cead1ba207e7297b931d/port/src/port_netlink.c)
soporta Steam/directo/relay, simulación de lag/jitter/pérdidas/cortes y cambio
de dirección del peer después de silencio. No son funcionalidades ausentes.

ShiningSource organiza cooperación de hasta cuatro jugadores con bloques de
24 bytes etiquetados, barreras de mapa, incorporación en cambios de mapa,
retención temporal del personaje y reglas de admisión del retorno. Las batallas
VS de dos jugadores tienen otros puntos irreversibles y necesidades de rollback.

La transferencia razonable es una matriz: pérdida durante handshake/transición,
corte/retorno, dirección nueva, controles duplicados/atrasados y retorno después
de un punto irreversible. Cada espera debe terminar y ambos pares deben admitir
o rechazar al retornante coherentemente. Separar estado reemplazable de mensajes
fiables sólo cuando el protocolo real lo justifique. No trasplantar el
join-in-progress cooperativo ni eliminar rollback. Ni los 23509 labels ni la
nueva conexión informada revalidan todos esos escenarios de recuperación.

## Estado del traspaso

Este documento deja propuestas revisables, fuentes fijadas y límites conocidos.
No aplica correcciones a ReKHCoM, no publica una release y no certifica teléfonos.
La nueva confirmación Android/Windows es de conexión; el reporte de sonido
remoto sigue abierto y necesita su propia corrección y verificación.
