# Galería: retorno seguro y vistas previas

Decisión de implementación autorizada por el usuario: reproducir desde título o partida y regresar a la misma
lista, selección y pestaña. Una captura temporal en RAM conserva memoria del juego, hardware y SRAM; se toma y
restaura únicamente en el límite del bucle principal. Las escenas no evalúan logros ni escriben partidas. La
reproducción HD suspende el overlay mientras su pausa está activa y lo recupera incluso si el video falla.

1. Regresión real antes del arreglo: `test_scene_gallery.py` sobre main bf09cf0 reproduce evento 10, vuelve al
   título y falla porque nunca regresa a la lista (log en `<work>/gallery-regression-before/run.log`).
2. Compartir el visitante de memoria ya usado por rollback; guardar una sola captura temporal acotada y liberarla
   después del retorno. Conservar SRAM y excluir evaluación de logros durante la reproducción.
3. Separar entrada configurada A/B de pestañas; mantener selección al volver. Recuperar overlay al acabar HD.
4. Añadir cartel lateral con nombre y captura estática, sin tapar filas. Mantener una sola miniatura residente;
   nunca precargar videos completos ni decodificar en cada cuadro.
5. Verificar reproducción, salto, regreso y repetición; captura real del menú, fuentes y `check_sources.py`.


## Estado para revisión (26/10)

- Piloto de la matriz: **24/24**. Modos HD / GBA mejorado / GBA original, Sora y Riku desde partida, GBA natural,
  ReKHCoM con motor nativo y cancelación de pausa, ReKHCoM con HD y salto, HD con archivo inválido (dos intentos).
- Cada reproducción completada compara estado de juego, mapa, configuración, archivos de logros/escenas y versión/rango
  Re:CoM; todos iguales. SRAM del save de prueba intacta, sin `bios:`, crash ni acceso a un canvas liberado.
- Previews antes de reproducir: **199/199** fondos GBA del catálogo; un solo still HD bajo demanda (MF/ffmpeg), después
  de 12 cuadros de cursor quieto. 160×90 residente + caché en disco; se liberan lector, sample y buffer del decoder.
- Hallazgo regional: EU omite los dos eventos dummy 127/128. Los límites de progreso Riku y previews usan ahora constantes
  del enum; no se confunde el prefijo numérico del símbolo con su valor. El arnés conserva IDs canónicos en los reportes
  y traduce a EU al invocar el ejecutable.
- Catálogo de 194 eventos no dummy + cinco películas. Tres listas GBA/HD/ReKHCoM: motor cuando no hay HD importado;
  mapeos, extras y offsets son los mismos de gameplay (`PortHdGalleryMovieName`, `MovieFile`, `PlayExtras`). Las colas
  de película ya contenidas en un evento HD completo no aparecen dos veces. No se cambia `hd_scenes` ni `hd_movies`.
- Compatibilidad: sin soporte de checkpoint (web), GBA se reproduce desde título y vuelve a título/lista; desde partida
  se muestra la limitación. Extracción de still HD previa disponible en Windows y Linux con decoder; los hosts sin ese
  extractor conservan el marcador, no se afirma soporte inexistente.
- **Pendiente antes de afirmar cierre general**: matriz grande sobre el SHA final de integración; pilotos dedicados de
  películas/extras, QA a 1920 y ultrapanorámico, y revisión independiente. No es evidencia de todas las combinaciones.
- Datos locales: `<work>/gallery-matrix-pilot2/{manifest.json,results.jsonl,logs,shots}`. 24 casos, dos reproducciones por
  caso, salida 0 y comparaciones registradas; los MP4 de codec son fixtures sintéticos, no footage del juego.

## Revisión posterior del candidato

- Cuatro pilotos adicionales pasan sobre el ejecutable EU SHA256 `78150ce92df6bb3e56629faac1c979e0b66f219b75d6e63082cd4b3f3b494406`: evento 56 con HD corrupto y fallback nativo; intro 513 nativa con salto; película 514 nativa con final natural; final Riku 517 HD cancelando la pausa. Dos reproducciones por caso, estado/SRAM intactos. Evidencia en `<work>/gallery-movie-pilots`.
- Un HD corrupto en ReKHCoM ya no oculta su cola cinematográfica nativa: se invalidan las entradas que comparten recurso, se reconstruye la lista y se conserva el evento seleccionado. El piloto confirma que vuelve a estar disponible el alias 515.
- La auditoría incluye metadata Re:CoM y nombres oficiales de logros EU/US/JP; esto amplía las huellas comparadas, sin afirmar ejecución de las tres regiones. El fallback sin checkpoint reinicia la captura para cada escena.
- El arnés limita los renderers a dos, fuerza un hilo PPU y rechaza reanudar resultados con otro ejecutable, ROM, save o fixtures. Cada resultado registra el SHA del binario.
- Cuatro pilotos de extras pasan con Sora/Riku y versión lógica Re:CoM desactivada/activada, aun con `hd_scenes=hd_movies=0`: la galería usa HD sin cambiar preferencias, y los extras 001/614 aparecen solamente con Re:CoM activo. El primer intento con SRAM totalmente vacía detectó inicialización legítima del arranque; el arnés exige ahora un save inicializado y los cuatro casos se repitieron sobre la copia estable. Evidencia en `<work>/gallery-extras-pilots`.


## Pósteres tras probar el pre privado (26/10)

- 199 PNG portables 360×240 producidos una sola vez con el motor real: 188 durante diálogo estable, seis escenas sin texto y cinco películas. Sus callbacks omiten ventana, retratos y flechas; permanecen personajes y decorado. Las películas omiten únicamente su OAM de subtítulos. No se cubren rectángulos ni se filtran los actores/objetos del escenario.
- Los cinco filmes se inspeccionaron. Riku usa frame 450: camina con Mickey, en vez del paisaje inicial. Ningún PNG es monocromo; catálogo aproximado 3,24 MB, un póster residente. El manifiesto conserva SHA, región y frame; cada packager rechaza un catálogo parcial o cambiado.
- generate_scene_posters.py usa exe/ROM compartidos, datos privados y un proceso sin ventana/audio por vez. Solo es una preparación explícita: abrir el menú no ejecuta escenas ni procesos. El resolver de producción de nombres canónicos se verifica con EU/US/JP, incluidos los dos dummy omitidos en EU.
- HD lee PNG importado 640×360 (360p, bajo el máximo 480p). Esa ruta no abre un decoder. Instalaciones antiguas sin PNG disponen de un único trabajo de un frame en Windows/Linux fuera de Draw. Otros hosts leen PNG portables; no se afirma extracción donde no está implementada.
- Cerrar/cambiar selección invalida generaciones sin esperar al codec; se recoge el trabajo al terminar y gana la selección más reciente. Solo Quit hace join antes de cerrar SDL. SourceReader síncrono limita ocho muestras, sin garantía de plazo absoluto dentro del codec; coordinación decidió mantener ese fallback y no prometer un timeout inexistente.
- La textura del cartel HD llega a la resolución de salida, evitando 640→90 píxeles lógicos→pantalla ampliada. Sentinel -2 comparte orden, Fill/Shade y fades; se libera al cerrar o cambiar renderer. GBA mantiene proporción 3:2 y filtrado de píxel.
- Probes: 199 PNG/SHA/dimensiones; resolver EU/US/JP; SDL preserva píxeles adyacentes y textos/markers, fades negro/blanco/mitad, modal parcial, cambio de renderer y cleanup. Worker simulado de 400 ms: cerrar tarda menos de 1 ms, reabrir descarta la imagen vieja, gana la nueva selección, worker/cache únicos y recursos recogidos.
- La matriz 2952 se canceló por instrucción del usuario para priorizar el candidato privado; no se afirma haberla ejecutado.
