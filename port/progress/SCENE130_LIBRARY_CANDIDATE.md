# Biblioteca de Bella: propuesta 130 en runtime

La petición «no la extendiste pues» corresponde al original Atlas
`c1ab45945afe471fb5f3f6f4b089e1e6` y a su hija offline pendiente
`e0a2151657fa4f15a4b9b7dc798fd28a`. Esta receta implementa esa propuesta
con coordenadas y píxeles del cartucho local: estanterías en traslaciones de
(64, ±32) y alfombra sobre la retícula (96,48)/(96,-48). El escritorio
permanece único. Ningún bitmap ROM se añade al repositorio.

Es una **candidata optativa**, habilitada por `KHCOM_PRELIMINAR_V1=1`.
No se añade 130 a la lista de aprobadas ni se modifica ninguna valoración.
Los hashes de BG3/BG2 son `0x0fdacfc6` / `0xfee677ce`, mapas de 512².
La captura EU se inicia por el menú de eventos **0:127**, confirmado mediante
`test_events.event_lists()`; no por el índice 130 del menú.

El muestreador conserva primero los píxeles originales ocupados y sólo
extiende sus zonas vacías fuera del viewport GBA. Mantiene índices de paleta
vivos para fundidos. No usa el slot ROM ausente 1023 como donante. El estado
VRAM de la escena en ejecución conserva la interpretación propia del motor.
La muesca verde original junto a la estantería izquierda también aparece
en la propuesta offline y sigue conservada; no se presenta como reparada.

Validación local EU32, un proceso a la vez, prioridad BelowNormal:

- Fixture C: diagonales de estanterías, retícula completa y división negativa.
- Resolver C: imagen de 1024×768 exactamente igual a la propuesta offline;
  139.324 píxeles originales distintos del fondo conservados.
- Preview: un PPU compilado/enlazado, 356 objetos de release reutilizados sin
  modificación; hashes de archivos fuente y ejecutable registrados.
- Vista Extendida: frames 300/600, escalas 2 y 3, cuatro pares antes/después;
  núcleo GBA idéntico en todos. Mismo ejecutable con/sin SKIP=130.
- Original: frames 300/600, imagen completa idéntica con candidata on/off.
- Predeterminado: frames 300/600 a escala 2 iguales al control sin receta;
  la candidata pendiente no se activa por defecto.

Evidencia privada en `pruebas/atlas-final-20261105/`:
`LIBRARY130-C-RESOLVER-CHECKS.json`, `LIBRARY130-RUNTIME-CHECKS.json`,
`SCENE130-PREVIEW-BUILD.json` y `library130-view*-runtime-*.png`.
La revisión humana de la nueva hija runtime queda pendiente. No certifica
otras plataformas, todos los fotogramas ni resolución de defectos previos.

Defecto separado observado: Maléfica ya aparece abajo a la derecha en la
vista Extendida sin receta, frames 300/600, ausente del viewport Original.
`src/evt/events/130_hollowbastion_e1.inc:sEvent130Track4` coloca su primer
keyframe en (500,500), tiempo 1100, flags 2 (sin DEFER_SPAWN=0x2000), y el
siguiente en (359,240), tiempo 1140. `event_chara_0` crea inmediatamente el
objeto desde el primer keyframe salvo DEFER_SPAWN. Es staging previo visible
en los márgenes amplios, ajeno a esta extensión BG; queda pendiente en su
propia tarea y no se afirma que toda la escena esté finalizada.
