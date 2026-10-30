# Issue330: seguimiento de accesos y versiones, 30/10/2026

Revisión de los comentarios nuevos, sin atribuir una causa no demostrada ni
modificar la instalación del reportero. Base57a9acacf; informe Steam Deck
[330](https://github.com/MatyMeatBoy/ReKHCoM/issues/330) permanece abierto.

## Qué llegó ahora

- [Gameboy121](https://github.com/MatyMeatBoy/ReKHCoM/issues/330#issuecomment-6511468893)
  indica que añadir el script desde Steam Desktop le permitió abrir Gaming Mode.
  No aporta el mismo paquete, acceso, configuración o logs del equipo afectado.
- [JamesOnthelake](https://github.com/MatyMeatBoy/ReKHCoM/issues/330#issuecomment-6517799568)
  responde que el procedimiento le funciona para otros juegos pero no para éste,
  y pregunta por Launch Options. No adjunta campos del acceso ni logs nuevos.

No es confirmación de instalación antigua, fallo de configuración, SDL o X11.
La confirmación de otro reportero en459 continúa siendo un caso separado.

## Qué enseña el código actual

`ReKHCoM.sh --steam-help` genera un conjunto coherente:

```
Target: /bin/sh
Start In: "carpeta absoluta del juego"
Launch Options: "ruta absoluta a ReKHCoM.sh"
```

Los argumentos opcionales siguen al script. Esa línea de Launch Options no se
puede copiar aislada a un acceso cuyo Target ya sea el propio script: allí sería
un argumento adicional, no el programa que ejecuta `/bin/sh`. La documentación
ahora explica la relación de los tres campos.

El registro de escritorio actual usa rutas absolutas. No edita `shortcuts.vdf`
ni los accesos que Steam ya conservaba. Actualizar o extraer un paquete en otra
carpeta no repara esos accesos: **un acceso obsoleto es una hipótesis plausible**,
distinta de que una preferencia antigua del juego cause el fallo.

El wrapper migra sólo las claves de runtime una vez por versión. Una elección
manual posterior sigue siendo válida; `--safe-startup` es temporal y no debe
reescribir preferencias, incluso si están desfasadas. Eso ya tiene regresiones.
No procede borrar ajustes, partidas, ROMs o importaciones para «limpiar» el caso.

## Comparación mínima que aún falta del equipo

1. Identificar el paquete realmente lanzado mediante la ruta del acceso y logs
   actuales, no por la carpeta que el jugador cree haber actualizado.
2. Contrastar juntos Target/Start In/Launch Options con `--steam-help` de esa
   misma carpeta. Mantener una sola prueba nativa, con Proton forzado desactivado.
3. Verificar que el intento fallido renueve el log en esa carpeta. Si existe,
   localizar la última etapa; si no, comprobar entrada al wrapper y escritura.
4. Sólo después contrastar configuración antigua y nueva con el mismo paquete
   en copias aisladas. Mantener instalación y partidas originales.

El log antiguo alcanzaba carga de host/guest y se detenía antes de localizar la
frontera SDL/ventana/renderer. El aviso ignorado de ELFCLASS32 del overlay no
demuestra por sí solo la causa. Los marcadores nuevos permitirían separar etapas;
no ha llegado todavía su resultado físico en este Deck.

## Verificación local de esta pasada

Seis consumidores existentes del wrapper real pasan con Git sh en Windows:
ruta manual y ruta segura con argumentos, registro sin ROM y relocación,
migración una vez por versión con elección manual posterior, modo seguro sin
cambiar preferencias antiguas y reemplazo de configuración fallido sin perder
el archivo anterior. Se ejecutan con binarios mock: no certifican SteamOS,
Gamescope, GPU ni un arranque físico del juego.

El primer intento con el entorno Windows sin adaptar falló por `dirname` ausente
del PATH de Git sh; los fixtures escritos por Python también necesitaban LF.
Se preservó el intento y se adaptó sólo el arnés privado (PATH y escrituras LF),
sin cambiar el wrapper de producción para forzar un resultado verde.

Evidencia privada: `pruebas/issue330-shortcut-followup/`, script y logs retenidos.
No se enviaron comentarios al reportero ni se cerró el issue.
