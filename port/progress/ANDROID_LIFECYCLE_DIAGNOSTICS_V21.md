# Android lifecycle diagnostics — 27/10/2026

Carril `android-lifecycle-diagnostics-v21`, owner `codex-android-lifecycle`.
Instrumentación para el repro e71 API34 assets-paths; no cambio de política de
recreación, no recovery/checkpoint, no ALLOW_RECREATE, no flags privados.

## Captura y límites

LauncherActivity y GameActivity registran antes de super onCreate/onPause/
onStop/onDestroy. PID, rol, sesión estable del proceso, instanceID nuevo por
Activity, reloj wall/monótono, restoredBundle booleano, isChangingConfigurations,
getChangingConfigurations en hex32 e isFinishing. No registrar Bundle/Intent,
rutas, URI, ROM, configuración completa, datos de juego ni mensajes de excepción.
Runtime Game se registra cuando getLibraries selecciona host64 o native32-eu/
us/jp; pending antes de esa selección. Es selección, no prueba de boot completo.
Launcher se identifica como launcher. El helper no conserva Context/Activity.

Logcat tag ReKHCoMLifecycle conserva además queued/disk_failed. Disco:
files/lifecycle-launcher.log y files/lifecycle-game.log, cada uno actual+.1..5,
16KiB porarchivo (hasta96KiB porrol). La rotación es propia, una vez al arrancar
el writer del proceso y al llenarse el archivo. No toca kh_com*.log ni comparte
prefijos/rotaciones con native. Las Activities de un proceso comparten writer;
los procesos launcher/:game escriben prefijos distintos en files/ de la app.
Normal/smoke/optiport tienen su propio sandbox/identidad como antes.

Callbacks sólo formatean campos acotados, ofrecen a cola32 (+1activo) y escriben
logcat; no hacen fileIO, esperas, flush/fsync ni join del writer. Daemon único,
rechazo inmediato con contador dropped. El worker hace E/S/rotación y falla
cerrado para diagnóstico: nunca aborta ni modifica transición/juego. Si encuentra
directorios o evidencia previa sobredimensionada en nombres reservados, no los
borra/trunca; desactiva sus escrituras y mantiene logcat. No prometer durabilidad
si System.exit/muerte mata el worker antes del append; no añadir espera para
ocultarlo. El timeout/join que ya haga SDL sigue sin tocarse.

## ShareLog y privacidad

El ShareLog existente del GameActivity incorpora primero un resumen lifecycle
≤6KiB leyendo sólo tails de1KiB de12nombres fijos. Filtra por el formato de
campos permitido, ignora texto arbitrario/filas parciales y no espera al writer.
Después mantiene logs nativos existentes, con lectura acotada28KiB porarchivo;
el texto total del Intent queda limitado64Ki caracteres (aprox128KiB UTF16,
más overhead). Excepciones de lectura no incluyen mensajes/rutas. Este helper
no añade secretos/ROM/config al resumen; los logs nativos ya existentes siguen
siendo diagnósticos compartidos por acción expresa del usuario, no ROM/config
binarios. No se envía nada automáticamente.

Limitación explícita: shareLog se invoca desde la caja de error del host64.
Tras una salida normal y destrucción deGameActivity no existe hoy un botón de
compartir ese historial desde el launcher. Este PR no rediseña ese UI ni finge
que un usuario de release puede leer su sandbox privado; en pruebas smoke el
coordinador obtiene archivos/logcat por sus medios existentes. La mejora de
exportación posterior es un pendiente separado.

## Pruebas y siguiente reproducción

`python port/tools/test_android_lifecycle.py` compila Java puro y ejercita:
instancia/sesión, rol/PID, runtime/flags0x80000000, rotación de sesión y tamaño,
actual+5, cola/tamaño/summary/share, campos no permitidos y directorio bloqueado
preservado. App+SDL Java compila contra android.jar35 con source/target8; no
APK/Gradle/native build. No callbacks Android reales ni emulador ejecutados.

Próximo integrador: APKsmoke deSHA exacto, cold-install inmediato frente45s;
conservar native logs, helper logs actuales/anteriores y logcat. Exigir primera
imagen/avance y correlacionar PID+session+instance:create→runtime→pause→stop→
destroy(changing/configmask/finishing)→nuevo create; distinguir exit0 pedido
porlifecycle de crash. Luego forzar recreación en título/campo/combate/HD/save,
comprobar SRAM/config preservados; no afirmar resume transitorio todavía.
No atribuir este mecanismo a272 ni RAM sin repro del estado del reportero.

Recuperación real permanece pendiente porbackend: handshake de checkpoint en
frontera segura antes de teardown, ack/commit atómico, identidadROM/build/ABI/
base/layout y restauración sin handles SO/avance duplicado ni pisar slots.
Android32 tiene stubs de quickstates; guest64 tiene estados rawpointer pero no
un protocolo Activity→checkpoint→resume. El logger sólo proporciona causalidad.
