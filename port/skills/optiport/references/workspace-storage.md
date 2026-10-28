# Proyecto ordenado y almacenamiento acotado

Aplicar al ordenar un proyecto, reducir su uso de disco o preparar entornos de pruebas. Optimizar espacio y comprensión humana conservando código, documentos, resultados y recuperación útil. Una reducción de almacenamiento no demuestra que la aplicación ejecute más rápido.

## Un mapa que una persona pueda entender

Reutilizar las convenciones existentes y acordadas del proyecto. Mantener un mapa breve en su entrada con ubicación, finalidad, qué abrir y qué se puede regenerar. No reorganizar internamente un repositorio sólo para embellecerlo: imports, rutas relativas, IDE, CI y herramientas pueden depender de su estructura.

| Ubicación conceptual | Contenido y decisión |
| --- | --- |
| Código y `worktrees/` | Repositorio base y checkouts de tareas identificados por propósito. Reunir los secundarios en una carpeta; mantener la base donde Git y el proyecto la requieran. El nombre concreto es una convención local. |
| `releases/` | Paquetes finales identificados por versión/plataforma, manifiesto y hashes. Separar las pruebas de una entrega publicada; no reemplazar esa entrega al regenerar un build. |
| `pruebas/` | Reproducciones y resultados por tarea y SHA: comando, log, resultado y evidencia pertinente. Separar ejecuciones temporales de evidencia que debe conservarse. |
| Assets, documentos y entradas | Una ubicación canónica para originales. Compartir entradas inmutables sin copiarlas por test; usar copias privadas para edición, configuración, estados o guardados. |
| Toolchains y cachés | Compiladores/SDK compartidos por versión y arquitectura; cachés regenerables con presupuesto y política de retención, fuera de cada checkout. |
| Respaldos | Recuperación identificable y comprobada, distinta de builds y cachés. No trasladar toda la basura a otro disco y contabilizarlo como espacio eliminado. |

Un acceso directo puede facilitar la entrada al proyecto sin duplicar sus datos. En Windows se pueden distinguir carpetas mediante iconos existentes y `desktop.ini`: usar `[.ShellClassInfo]` e `IconResource` con un recurso estable, conservar otros ajustes, marcar el INI como oculto/sistema y la carpeta como personalizada mediante su atributo de sólo lectura. Comprobar el resultado en Explorer. Mantener nombres y un mapa textual: el icono es una ayuda opcional, no la única identificación. Evitar rutas a iconos dentro de un worktree temporal y no convertir la personalización visual en dependencia de ejecución.

### Evidencia actual e histórica

Mantener un índice de fixtures/capturas con binario y fuentes, generación del importador/recetas y contenido, arquitectura/región/opciones, propósito, hashes y estado actual o heredado. Al sustituir una familia de recursos o su productor, conservar la evidencia anterior y marcar su alcance histórico y la referencia que la sustituye; archivar no implica borrar ni hacer pasar una captura antigua por el resultado nuevo.

Reutilizar un resultado anterior sólo cuando sus condiciones y cobertura sigan siendo equivalentes, con esa justificación registrada. Una carpeta llamada `latest`, un binario recién compilado o una fecha de copia reciente no actualizan los assets de un fixture. Antes de reutilizar una importación, aplicar la [verificación de contenido y calidad efectiva](release-readiness.md#código-contenido-importado-y-calidad-efectiva). Conservar los paquetes ya entregados y sus hashes fuera de staging mutable; las notas o validaciones posteriores llevan su propia identidad.

Conservar los auxiliares prometidos fuera de la limpieza del builder y verificar su ruta y hash retenidos después del cleanup. Haber descargado o calculado el hash de un archivo debug no prueba que siga disponible. Si se perdió, registrar ese límite y buscar una copia exacta; no afirmar retención a partir del log ni regenerar/sustituir una entrega final para ocultar el auxiliar faltante.

## Medir antes de limpiar

Registrar las raíces autorizadas de cada volumen, consumidores activos y áreas protegidas. Separar **tamaño lógico**, **espacio realmente asignado**, **archivos únicos** y **espacio recuperable**. La suma mostrada por una búsqueda recursiva no prueba cuánto disco se liberará.

- Inventariar junctions, symlinks y otros reparse points sin recorrer sus destinos repetidamente; registrar la referencia y comprobar su destino por separado cuando corresponda. No sumar un enlace como otra copia ni atravesarlo al limpiar.
- Identificar hardlinks por identidad de archivo y volumen, no por nombre. Contar una vez su almacenamiento; retirar una de sus rutas no libera los datos mientras otra los referencie. La compresión o los archivos dispersos también separan longitud lógica de ocupación física.
- Agrupar candidatos por tamaño y verificar su contenido con un hash antes de deduplicar. Un hash del flujo principal no prueba equivalencia de permisos, atributos o flujos adicionales: conservar/comprobar los metadatos necesarios y los consumidores antes de retirar la copia. Mantener manifiesto de fuente conservada, rutas afectadas y verificación; un nombre, extensión o SHA de Git no demuestra igualdad de archivos ignorados.
- Un TAR no es redundante sólo porque exista una carpeta con su nombre. Comparar **todos** sus miembros regulares por tamaño y SHA256 con archivos retenidos y registrar rutas, permisos/ejecutabilidad, fechas, ownership, links y cabeceras PAX/globales necesarias para reconstruirlo. Conservar el contenedor si un miembro difiere, no tiene copia o su metadata/relaciones no pueden preservarse. Un outbox con cabecera vacía puede contener datos después: comprobar su contenido completo antes de clasificarlo como vacío o completamente cero.
- Compartir por hardlink sólo archivos que todos sus consumidores tratarán como inmutables. Una escritura en cualquiera de sus rutas modifica el mismo archivo: excluir código editable, documentos, bases de datos, configuraciones y guardados. Si una herramienta reescribe un resultado en su sitio, conservar su copia privada o usar un mecanismo real de copia al escribir compatible con ella.
- No inferir que un SDK, fixture, snapshot o archivo `dummy` es prescindible por su nombre. Comprobar referencias, vigencia y reconstrucción. Preservar WIP, ignorados necesarios, entradas originales y evidencia antes de retirar copias.
- Un directorio marcado como «borrable» puede contener releases protegidas intencionadamente mediante atributos de sólo lectura. Separar esos paquetes y sus hashes en la ubicación protegida antes de retirar basura. No forzar el borrado ni cambiar sus atributos para cumplir una cuota; la igualdad del contenido extraído tampoco autoriza reemplazar una entrega final con otra identidad o firma.

En Windows, `GetCompressedFileSizeW` distingue almacenamiento comprimido/disperso, pero para archivos ordinarios puede devolver su longitud lógica: no incluye todos los metadatos ni el slack de clusters. Etiquetar esa métrica y su límite, sin confundirla con el espacio libre real del volumen.

Comparar ocupación antes/después con el mismo alcance y unidad, distinguiendo copias temporales, candidatos de migración y estado final. Fechar la ventana de medición y señalar actividad concurrente: una foto de archivos que cambian es provisional y la variación de espacio libre de todo un volumen no se atribuye íntegramente a esta tarea. Informar por volumen y distinguir ahorro conseguido de ahorro potencial. Registrar errores de lectura y cobertura incompleta; no presentarlos como cero bytes.

El resultado puede superar el objetivo si siguen siendo necesarios originales, SDK o evidencias. Explicar los bloques conservados y el tradeoff concreto; no descartarlos ni recomprimir originales con pérdida para prometer una cifra. El objetivo de espacio no sustituye los requisitos de conservación y fidelidad del proyecto.

## Consolidar sin romper rutas

Esperar a que terminen los consumidores afectados; usar las herramientas de Git para mover o archivar worktrees. Antes de una operación recursiva en Windows, resolver y verificar las rutas absolutas de origen y destino dentro del alcance autorizado. Un checkout limpio puede contener ignorados importantes; conservar también los commits de un HEAD separado mediante una referencia recuperable.

Actualizar launchers, IDE, scripts, referencias de assets, configuración de emuladores y ubicaciones de resultados. Una referencia de compatibilidad puede conservar temporalmente una ruta antigua; registrarla en el mapa y no contabilizarla como otra copia. Comprobar el arranque o flujo consumidor que realmente usaba cada ruta movida. Mantener la instalación personal del usuario fuera del trabajo.

Aplicar [coordinación](coordination-workspaces.md) para conservar tareas activas y PR en revisión. Tras entregar una tarea, tomar la siguiente independiente mediante la autoridad existente; no reservar toda la cola ni crear un checkout con toolchain y assets completos por cada intento. Reutilizar un checkout libre cuando sea seguro, conservando la referencia y evidencia de la tarea anterior.

## VM y emuladores con margen justificado

Medir por separado capacidad virtual, espacio usado dentro del invitado, tamaño físico del contenedor y crecimiento temporal del trabajo real. **Un contenedor pequeño en el host no demuestra que el invitado quepa en una capacidad virtual menor**: puede haber bloques libres pendientes de recuperar, compresión o una cadena de discos base/overlay.

Dimensionar a partir del uso útil del invitado, el pico simultáneo de compilación/importación, las cachés que se conservarán y una reserva de crecimiento razonada. Elegir el margen según el trabajo medido; no imponer un límite fijo ni un porcentaje universal a todos los entornos. Un disco dinámico grande no reserva necesariamente toda su capacidad, aunque siguen haciendo falta límites y espacio libre real en el host.

- Reutilizar una base por sistema/arquitectura y toolchain. Usar overlays o snapshots por tarea cuando proceda, con exclusión de escritura y ownership claros; no copiar una VM completa por test. Conservar las dependencias de cada cadena antes de mover o retirar su base.
- Acotar cachés del invitado, paquetes temporales, directorios compartidos, snapshots de RAM y discos de salida. Dimensionar el outbox al resultado esperado con margen y liberar temporales al cerrar, cancelar o fallar, conservando logs y reproducciones necesarias.
- Para recuperar almacenamiento físico, comprobar limpieza de cachés y descarte/TRIM compatible, o compactación fuera de línea según el formato. Verificar integridad y contenido; comprimir una imagen no reduce por sí mismo la capacidad que ve el invitado.
- Una base y su copia pueden compartir UUID de filesystem/partición. Si ambas se conectan antes del arranque de mantenimiento, la selección por UUID puede montar como raíz el disco objetivo equivocado, incluso en sólo lectura. Seleccionar una raíz inequívoca y comprobar con `findmnt`, `lsblk` y la identidad real de los dispositivos qué disco contiene cada montaje; un UUID compartido o un nombre de dispositivo aislado no basta. Conectar el objetivo después de arrancar el entorno de mantenimiento es una opción. Confirmar que el filesystem objetivo está desmontado antes de cualquier modificación.
- Separar las herramientas del invitado de mantenimiento de las dependencias del artefacto que se valida. La ausencia de una utilidad diagnóstica en una imagen mínima no demuestra una regresión del producto ni pérdida de su SDK: preparar el entorno de mantenimiento pertinente y verificar las dependencias reales del artefacto por separado.
- Para reducir capacidad, trabajar primero sobre una copia recuperable, con el invitado detenido y el sistema de archivos objetivo desmontado. Confirmar que el formato y filesystem permiten reducir; comprobar/reducir el filesystem y después su partición preservando el inicio y los datos de arranque. Antes de reducir la capacidad virtual, verificar que ningún extremo de partición supera el tamaño final. Reducir el contenedor mediante herramientas compatibles y ajustar las estructuras del formato/tabla, incluida su copia final cuando corresponda. Nunca truncarlo basándose sólo en sus bytes ocupados.
- Antes de sustituir la imagen conservada, comprobar integridad, arranque, montaje, SDK y un trabajo representativo con su pico de espacio. Retirar la copia anterior sólo después de verificar recuperación y política de retención. Si no es viable reducir con seguridad, conservarla y explicar el límite concreto.

## Aplicación en ingeniería

Separar originales y resultados autoritativos de teselaciones, previews, cachés de solver, índices y temporales. Una caché puede ser regenerable sin que lo sean el documento que la originó ni los resultados aprobados. No deduplicar modelos editables mediante hardlinks.

En CAD/BIM/CAE, verificar referencias enlazadas, geometría, unidades, coordenadas, precisión, materiales/metadatos relevantes, undo/redo y exportación tras mover o cambiar cachés. En GIS, incluir CRS, datum, transformaciones, georreferenciación y precisión espacial conforme a los criterios existentes. Los resultados de solver conservan su configuración y tolerancias validadas. Aplicar [invariantes del dominio](domain-aware-optimization.md): optimizar la presentación o el almacenamiento sin cambiar el modelo ni aceptar más error sin autorización.
