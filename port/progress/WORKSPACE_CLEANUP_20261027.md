# Limpieza y organización del workspace — 27/10/2026

Pasada de almacenamiento autorizada por el usuario, prioritaria antes de
continuar funcionalidades. Los archivos privados de recuperación permanecen
locales; este registro no publica partidas, ROMs, imágenes de VM ni datos de Atlas.

## Estructura y conservación

- Los secundarios de ReKHCoM y OptiPort están reunidos en
  `KHCoMPC/kh-worktrees/`; los repositorios base conservan su ubicación.
- Del inventario inicial de 40 worktrees de ReKHCoM se retiraron 32. Sus
  40 HEAD exactos siguen recuperables mediante referencias o checkouts.
  Los checkouts nuevos de trabajo activo se cuentan aparte.
- Los ignorados necesarios de 27 checkouts retirados se conservaron en ZIP:
  CRC íntegro y 1733 hashes SHA256 comprobados. Los otros cinco no tenían
  ignorados. La lista inicial de WIP del repo base y del ajuste de cartas
  se conserva; no existía una captura previa de hashes de ese WIP.
- `releases/` reúne paquetes finales y hashes, incluidos los originales
  protegidos de la 2.0. El ZIP 2.1 conservado mantiene su SHA256.
- Las compilaciones y pruebas históricas de los tres checkouts grandes
  están en `pruebas/workspaces/`, con junctions desde sus rutas `build`.
  Los datos de Atlas permanecen en su ubicación original; su API responde
  desde el checkout trasladado con el mismo directorio de datos.
- El directorio físico pesado de D: es `D:/01Claude/ReKHCoM-build/`.
  El antiguo `D:/ReKHCoM-build/` mantiene compatibilidad mediante enlaces.
  Los AVD y temporales que aún eran físicos allí fueron reunidos en el destino.
- Los mapas de entrada y los iconos de carpetas distinguen código, paquetes,
  pruebas, originales, herramientas y recuperación. Los nombres textuales
  siguen siendo suficientes para identificar cada ubicación.

## Reducción de almacenamiento

La cifra aparente de 142 GB incluía recorridos repetidos de vídeos enlazados.
El recuento correcto no sigue junctions y cuenta los hardlinks una sola vez.
Bytes lógicos y espacio ocupado se registran por separado; los metadatos y
la actividad concurrente impiden atribuir toda variación del volumen a esta pasada.

- Se compartieron 274 archivos idénticos tras comparar hashes y comprobar
  identidad/conteo de enlaces. Las entradas tratadas son recursos inmutables
  de pruebas; se conservan sus rutas. No se aplicó esa política a partidas,
  configuraciones editables ni documentos originales.
- La compresión NTFS conserva los bytes lógicos de imágenes Android y
  resultados regenerables. Se mantienen los AVD utilizados y el SDK de
  compilación; se retiraron las imágenes de emulador 33/35 sin AVD consumidor.
- Se retiró un snapshot de RAM Android innecesario y se configuró arranque
  en frío. Los datos de usuario de los emuladores se conservaron.
- La carpeta antigua de descarte fue retirada después de conservar
  30331 archivos de evidencia/partidas en un archivo verificado y trasladar
  sus paquetes finales protegidos a `releases`.
- Ocho TAR tenían sus 2310 miembros regulares idénticos en archivos retenidos;
  un outbox de 1 GiB era completamente cero. Se registraron nombres, hashes,
  permisos y metadata antes de retirarlos: 2,26 GiB de contenedores redundantes.
  Otros cuatro TAR se conservaron por diferencias o archivos no encontrados.

## VM dimensionadas con el uso real

| Entorno | Capacidad anterior | Capacidad final | Contenedor anterior | Contenedor final |
|---|---:|---:|---:|---:|
| Compilación PS5 | 40 GiB | 16 GiB | 10,47 GiB | 4,55 GiB |
| Linux gráfico | 40 GiB | 4 GiB | 1,54 GiB | 0,70 GiB |

Las bases Debian de 3 GiB se conservan para las herramientas que las consumen.
Las imágenes resultantes son independientes de las rutas antiguas de backing.

Se trabajó sobre copias recuperables con el filesystem objetivo desmontado:
comprobación de ext4, ajuste de filesystem/partición cuando correspondía,
reducción del contenedor y reparación/verificación de GPT. Se comparó la
huella SHA256 del SDK de PS5 y de las bibliotecas Linux antes/después.
Las copias de migración se retiraron tras verificar las imágenes conservadas.

La PS5 arranca, monta su raíz, conserva el SDK y ejecuta Python/clang/ccache;
quedan aproximadamente 4,20 GiB disponibles dentro del invitado. Linux arranca
en 4 GiB, conserva las bibliotecas de gráficos de 32 bits y abre el framebuffer
Xvfb; quedan aproximadamente 1,66 GiB disponibles. No se repitió una compilación
completa de consola ni se atribuye una mejora de velocidad a esta limpieza.

## Prevención y coordinación

[PR #362](https://github.com/MatyMeatBoy/ReKHCoM/pull/362), integrado en
`e19abea400f58219876b6a3cd7c61d2664c9fcbc`, crea discos persistentes nuevos de
16 GiB por defecto y omite el outbox de 1 GiB cuando no hay `--fetch`.
Las imágenes existentes conservan su capacidad y la evidencia de un fetch
fallido se mantiene. Cuatro regresiones semánticas verifican ese comportamiento;
los blobs integrados coinciden con los probados.

La cola remota `coordination/art-queue` sigue siendo la única autoridad:
claim de una tarea libre, PR pequeño con evidencia, integración con lock y
siguiente tarea independiente. Los checkouts temporales se retiran al integrar,
conservando ramas y evidencia. OptiPort incorpora estas decisiones también para
proyectos de ingeniería, con conservación de unidades, coordenadas, modelos,
undo/exportación y resultados autoritativos.

## Foto del espacio ocupado al cierre

Medición del 27/10 a las 12:39, America/Santiago: **28,72 GiB en C:** y
**39,13 GiB en D:**, 67,85 GiB en conjunto. Recuento sin seguir 102 reparse
points, con cada hardlink contado una sola vez; cero errores de lectura.
`GetCompressedFileSizeW` mide datos almacenados y no todo el slack/metadatos
del filesystem. La actividad concurrente puede modificar esta fotografía.

No se alcanzaron 30 GiB para el proyecto completo. Entre los bloques que
permanecen en D: están los originales del tráiler (~9,82 GiB), la importación
HD (~6,63 GiB), Linux (~6,31 GiB), los AVD (~4,75 GiB) y PS5 (~4,73 GiB).
Se conservaron entradas y producción originales, SDK y evidencias necesarias;
el cierre de esta pasada no autoriza descartarlos para cumplir una cifra.
