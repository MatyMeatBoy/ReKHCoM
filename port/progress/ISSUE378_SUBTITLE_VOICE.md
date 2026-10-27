# #378: subtítulos separados de la voz HD — 27/10/2026

Base `55b730619`. La voz Undub del cartucho y el audio de los vídeos HD son
opciones distintas: descargar/importar Steam en español no convierte en
japonés el audio de sus MP4. La fuente japonesa tampoco garantiza incluir ES.

Se reprodujo otro límite concreto del consumidor: con voz JP seleccionada,
subtítulos US en esa generación y ES presente sólo en la biblioteca EN, el
reproductor pedía ES pero terminaba cargando US. La ruta sólo buscaba dentro
de la generación del vídeo y en el directorio legacy global. El primer test
del `LoadSubs` productivo falló antes de cambiarlo (exit97, texto EN).

## Cambio mínimo y seguridad temporal

Se conserva el vídeo/audio seleccionado. Si falta la traducción en su propia
generación, el resolver puede usar texto realmente extraído de otra generación
activa/validada o del directorio global. Requiere una referencia existente de
la generación seleccionada y coincidencia de **todos** sus intervalos de cues:
igual número de filas y pares inicio/fin, en las unidades de30fps que consume
el reproductor. Se rechazan vacío, filas malformadas, tiempos fuera de0..65535,
orden inválido y más de256cues. No se supone compatibilidad por nombre,
duración del MP4, idioma declarado ni un prefijo de filas coincidente.

No se sintetiza/traduce texto, no se retiman cues y no se cambia a otro MP4.
Sin ES o sin referencia compatible se conserva el fallback existente. La
traducción de la misma generación mantiene sus tiempos propios. Legacy/Auto
conserva su ruta anterior; la comprobación cruzada se aplica a voz explícita.
No cambia importadores, archivos importados, ABI ni preferencias.

Para combinar JP+ES hacen falta ambas fuentes propias: importar una biblioteca
JP, conservarla, y disponer de ES extraído de la fuente española/EN. Después
seleccionar audio HD japonés y texto del juego español, con subtítulos HD
activados. El selector GBA Undub por sí solo no selecciona audio japonés HD.
Si sus cues no coinciden, esta corrección no mezcla esas fuentes a ciegas.

## Verificación

`test_hd_subtitle_voice_independence.py` ejecuta el `LoadSubs` real y el camino
guest `PlayFile` → adaptador Windows de `prepare_movie_core.py` → loader host.
Los límites de vídeo/SDL están simulados; sólo hay strings propios y headers
MP4 sintéticos en un subdirectorio nuevo de la carpeta privada de pruebas.
No lee ni modifica instalaciones, ROMs, partidas ni vídeos del usuario.

14casos: ES cruzado compatible, desfasado, ausente, sin referencia, traducción
local, legacy, global compatible/incompatible, distinto número de cues,
referencia JA, generación no activa, ES en generación movies sin el MP4 de esa
escena, rango inválido y texto vacío. Todos comprueban también que vídeo,
selección de voz y Undub permanecen iguales. RED antes; GREEN después.

Regresión de variantes EN/JP existente: PASS. Syntax-only de la fuente nativa
Windows32 y del guest x32 compartido: PASS, sin build completo. Se inspeccionaron
sólo tiempos de dos subtítulos propios legacy disponibles: ROP(3cues) y
zz_event_012(15cues), ES/US coincidentes; no se copió su texto al repositorio.

**Límite:** no hay fuente JP propia disponible aquí para confirmar la mezcla
visual/auditiva del caso del reportero. Los negativos JP usan fixtures explícitas.
La comprobación de intervalos es un filtro conservador, no una afirmación de
equivalencia de todo doblaje. No se declara resuelto el reporte en su equipo ni
se modifica la prerelease ya entregada; requiere prueba del siguiente artefacto.
