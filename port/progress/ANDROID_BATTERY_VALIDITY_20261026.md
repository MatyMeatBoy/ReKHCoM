# Baterías: retoma y validez de resultados (26/10/2026)

## Fallo Android explicado

El único resultado no satisfactorio de `khcom-claude-int/build/android-scenarios/report-us-jp.json`
es `us/battle-123-BATTLE_EVENT_AGRABAH_1`. Su evidencia del 25/10 en
`build/android-scenarios/cases/us-battle-123-BATTLE_EVENT_AGRABAH_1/kh_com_host.log` dice:

```text
the game image guest-us.macho is missing from the APK
```

El juego no llegó a arrancar (`kh_com.log` vacío). No demuestra un cuelgue del combate.
La app con solo EU era una entrada de prueba inválida. Se conserva la evidencia y el
resultado fallido; todavía hace falta repetir el caso con la app completa.

`test_android_scenarios.py` ahora detecta esa causa antes de considerar el cierre del
proceso un éxito. También rechaza un código de salida no nulo del host y un cierre sin
log del juego. Guarda la evidencia con `BAD-APK`, `GAME-ERROR` o `BAD-SCENARIO`, según
corresponda. Antes, los dos últimos podían producir un `ok` falso.

Validación offline: seis casos, cuatro fallaban antes del arreglo y los seis pasan
después; `python port/tools/test_android_scenario_results.py`. Comprueba los dos
comportamientos de la app incompleta (ventana de error abierta o proceso cerrado),
salida no nula, log vacío y salidas normales de los hosts nuevo y anterior.
`check_sources.py`: OK.

## Suite que sigue viva

El runner de `khcom-claude-int` arrancado el 26/10 a las 06:22 sigue trabajando,
sin relanzarlo ni cambiar su checkout. Los procesos Python anidados son wrappers del
intérprete, no runners duplicados. Confirmados:

- `scenes` y `events` EU/US/JP terminados; `wide` EU/US terminados.
- `wide` JP sigue avanzando. Faltan las etapas posteriores y Android.
- Informe de escritorio al empezar la retoma: 1257 logs escaneados, cero sitios
  `bios:` y cero crashes. Ese informe parcial no valida la versión final.
- Su checkout es `57524e45a`, anterior a la versión final. Las pruebas de lanzamiento
  se hacen aparte contra el commit final, sin atribuirles los resultados de este árbol.

## APK y siguiente acción

Se verificó con `zipfile` que
`khcom-claude-int/build/releases/ReKHCoM-v2.0-pre19-android-alpha-smoke.apk`
contiene `assets/guest-eu.macho`, `assets/guest-us.macho` y `assets/guest-jp.macho`.
Esto verifica el archivo, no la app instalada ni la ejecución del combate.

`run_batteries.py` arranca el emulador pero **no instala el APK**. Cuando llegue la
etapa Android, comprobar/instalar una app smoke completa y repetir Agrabah 123/124
EU/US/JP; no arrancar otro emulador en paralelo con las baterías de escritorio.
`--resume` solo omite resultados `ok`: el caso US123 fallido volverá a ejecutarse.
Para repetir solo estos casos sin reemplazar un informe de suite completo, importar
el módulo y asignar una carpeta `OUT` aislada, o conservar el informe anterior antes
de usar `--only`. No borrar la evidencia del fallo original.
