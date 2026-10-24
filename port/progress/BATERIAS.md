# Baterías largas (21/10/2026)

Para Claude y Codex: las sesiones se cortan por límite de tokens y la otra IA sigue. Las baterías tienen que
sobrevivir a eso y poder retomarse por cualquiera.

## Cómo se corren

```
python port/tools/run_batteries.py --detach    # arranca fuera de la sesión (WMI) y vuelve enseguida
python port/tools/run_batteries.py --status    # cuánto lleva cada paso y las últimas líneas del registro
```

- **Fuera de la sesión.** Un proceso hijo de la sesión muere cuando la sesión se cierra (Windows termina todo el
  árbol: adb da 0xC0000142, sh no puede hacer fork con 0xC000026B, ninja sale con 0x40010004). Eso pasó dos veces
  el 21/10 y no eran fallos del juego. `--detach` crea el proceso con WMI, fuera de ese árbol. Nunca correr una
  batería de horas como tarea en segundo plano de la sesión.
- **Pasos chicos.** Un paso por batería y versión (escenas EU, eventos EU, 16:9 EU, salas JP...), luego Android.
  Escritorio primero: con el emulador encendido no alcanza la RAM para las dos cosas.
- **Escenas antes que nada (21/10).** `scenes` corre en el escritorio los mismos menús y fixtures que el arnés de
  Android. Cada acceso suelto que el escritorio recupera (`bios:`) cierra la app de Android 64: el selector de
  eventos de depuración llegó así hasta el emulador. En minutos en PC en vez de horas en el emulador.
- **Al terminar y revisar** un paso (resultados leídos, fallos anotados o arreglados), sus carpetas por caso se
  borran si ya no se van a usar; quedan el `report.md`/`report.json` y los `.done` (regla del usuario: borrar solo
  lo que ya se usó y se comprobó).
- **Retomables por caso.** Cada caso terminado deja su resultado en disco (`result.json` en su carpeta de
  `build/eventtest/<versión>[_wide]` o `build/roomcases_<versión>`; Android en
  `build/android-scenarios/report-*.json`). Con `--resume` (lo pasan todos los pasos) se salta lo hecho, así que
  volver a lanzar `--detach` sigue desde el primer caso sin resultado. Una tanda completa deja
  `build/debugkit/<batería>-<versión>.done`.
- **adb.** Si adb.exe no arranca (0xC0000142 / 0xC000026B), se reintenta: no es resultado del juego.
- **Nombres de archivo** sin `:` (en NTFS crea un flujo oculto en vez de un archivo).

## Al tomar el trabajo

1. `--status`. Si ningún proceso `run_batteries.py` está vivo y quedan pasos, `--detach` otra vez.
2. Los resultados: `python port/tools/debug_kit.py report` (`build/debugkit/report.md`), cada `bios:` nuevo es un bug
   a arreglar en el juego (`KIT_DEPURACION.md`); Android en `ANDROID_64.md`.
3. Para cambiar qué se corre, editar `STEPS` en `run_batteries.py`.
