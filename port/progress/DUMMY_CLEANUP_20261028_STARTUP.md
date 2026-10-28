# Limpieza acotada de pruebas de arranque — 28/10/2026

La pasada terminó el 28/10/2026 a las 20:00:18 UTC. Se retiraron únicamente
151 archivos generados de pruebas ya cerradas: bytecode JVM/Python y ejecutables
independientes de sondas CPU. Antes de retirarlos se conservó un ZIP recuperable,
con inventario y SHA256 por archivo.

## Evidencia local y contabilidad

Los recibos permanecen en la raíz del espacio de trabajo, fuera del repositorio:

- `pruebas/dummy-cleanup-20261028-startup/RESULT.json`: resultado `PASS`, alcance y contabilidad.
- `pruebas/dummy-cleanup-20261028-startup/MANIFEST.json`: ruta original, miembro del ZIP, tamaño, SHA256 y fecha por archivo.
- `respaldos/dummy-cleanup-20261028-startup.zip`: respaldo de recuperación.

| Concepto | Bytes |
| --- | ---: |
| Longitudes de los 151 archivos retirados | 3 804 358 |
| ZIP conservado | 1 517 147 |
| Reducción neta de longitudes de archivos | 2 287 211 |

SHA256 del ZIP: `8ed83d752227f220535d761dbf846a588f26be2cd838710928c45f743313937c`.
La revisión independiente volvió a leer los 151 miembros del respaldo y verificó
sus tamaños y SHA256 contra el manifiesto, además del tamaño y SHA256 del ZIP.
La diferencia de espacio libre del volumen no se atribuye a esta pasada: puede
incluir actividad concurrente. La cifra neta anterior es contabilidad de archivos,
no una medida de bloques físicos recuperados ni una optimización de rendimiento.

## Límites de la pasada

Se conservaron fuentes, objetos nativos `.o`, cachés de compilación, SDK, registros,
capturas, ROM, SAV, configuración, recursos y paquetes publicados. Tampoco se
retiraron directorios de las pruebas de arranque activas. Los objetos nativos de
unos 32 MB quedaron fuera del alcance. El backend SDL denominado `dummy` es una
herramienta válida de pruebas y su nombre no lo convierte en un archivo descartable.
Esta documentación no ejecuta ni autoriza otra eliminación.

## Recuperación

Para recuperar una sonda, consultar su fila en `MANIFEST.json`, comprobar primero
el SHA256 del ZIP y extraer **sólo ese miembro a una carpeta nueva**. Verificar el
tamaño y SHA256 contra la fila antes de usarlo. Si hace falta devolverlo a su ruta
original, comprobar que no exista allí un archivo nuevo; no sobrescribir trabajo
posterior. El manifiesto conserva la ruta original y `mtime_ns` como referencia.
Los recibos y el ZIP son evidencia local; este PR sólo añade esta nota.
