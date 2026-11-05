# Escena081: última corrección humana

Claim `atlas-scene081-final-review`, owner `codex-atlas081-final`.
Base aprobada `8072e8379e404ddd9ba2eac73de50f3c` (rev20); último parent de
revisión `b39c0c96f13c45319956f1b5269c5b89` (rev4), envío
`20261029-134334-6efc5a`. PNG del envío verificado SHA
`69b9b24d81c7f98aa80498df7fa7fda6f73f6f5d836c9d04feb87ef73423a737`.

Las cuatro nubes corrigen la interpretación anterior: la parte indicada debe
seguir negra y conservar el borde original de la caverna. El lado opuesto está
bien. También se pide mejorar la presentación de los sincorazón.

Resolución y medio ya fijados por el proyecto: tiles8×8, píxeles nativos4bpp,
paleta original viva; juego GBA240×160 con extensión, sin nuevo raster/IA.
La interpretación elegida, para valoración de esta nueva hija, es:

1. Restaurar la identidad de todas las capas del remate nativo a la izquierda
   de x128 y su máscara negra; exterior x<0 negro. BG2 conserva sólo sus
   muestras originales, sin clonar pilares/props. El muestreo aprobado del
   lado derecho, x>=128, permanece exactamente igual.
2. En evento081 únicamente, ocultar la pose de espera472
   (`gEmy00L00Tiles`, `gEmy00Palette`) cuando queda totalmente fuera de GBA.
   La marcha474 (`gEmy00L02Tiles`) y todos los píxeles dentro de GBA siguen
   intactos. No usar una bandera global por paleta: hay seis tracks y dos
   oleadas con tres posiciones iniciales repetidas.
3. Pruebas C del muestreador real y del filtro de actor, comparación de núcleo
   y Original. Entregar únicamente una nueva hija pending con ANTES/SOLUCIÓN
   de capturas reales en el mismo encuadre, usando el Store activo. Las fuentes
   ROM y modelos offline permanecen internos; no heredan aprobación.

Investigación interna: `pruebas/atlas-final-20261105/scene081-final/` contiene
el snapshot revisado, sus IDs, fuentes ROM derivadas y un modelo de coordenadas
antes/después. No son capturas del juego ni entregas públicas.

Pendiente ejecución C RED/GREEN y captura runtime cuando se conceda ventana;
no se afirma arreglado ni validado en el motor. Sin cambios de ROM/SAV,
instalación, aprobaciones o versiones anteriores.
