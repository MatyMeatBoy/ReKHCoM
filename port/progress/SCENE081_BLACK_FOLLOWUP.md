# 081: arco decorativo bajo el remate — propuesta sin build

Seguimiento de `0bac2ac10b81462e910b3705bd3c6740`, revisión2, SHA256
`7ce45e84dae059c922d5dfb44b388a577320f4824928d8c7bfd736720abb2502`.
Nube `0dda90ce-59b5-48ef-a420-1d4e08bdfe9d`: «falta tapar con negro».
La versión anterior está aceptada e integrada; esta propuesta no hereda eso.

El fragmento flotante coincide exactamente en319píxeles nativos con el tramo
inferior de `gMapGmkMonstro03Tiles`, no con el fondo ni con los actores. Las
tres capas del mapa están vacías en x0..127/y256..511. Se omiten únicamente
los píxeles de esa decoración en ese sector, fuera del viewport GBA, dejando
visible el fondo negro existente. No se pinta un rectángulo RGB sobre actores
ni se cambian ROM, paleta, colisiones o entrada de sincorazones.

El gate exige evento081 regional, receta081 activa, latch válido, banco exacto
de Platform03 y OBJ de mundo. Usa las coordenadas BG1 del fotograma presentado.
Los otros objetos y eventos pasan intactos; Original nunca cumple el predicado.
El fixture de conformidad define el nuevo símbolo para una prueba posterior.

La comparativa usa la captura previa aceptada del fotograma300 y una simulación
del recorte, claramente rotulada. Calibración única3x del fondo en(1323,336);
cambia exactamente2871píxeles físicos, todos del arco indicado. NúcleoGBA y
resto de la imagen idénticos. Tres pruebas Python comprueban límites, el guard
de los38400píxeles nativos y registro del gate, sin compilador C.

**No hay build ni nueva captura del juego.** El dibujo OAM integrado, posibles
reemplazos HD, otras cámaras y controles Original ejecutados quedan pendientes
cuando se autoricen pruebas. La imagen es propuesta, no evidencia runtime del
código nuevo. Fuentes ROM internas y evidencia en
`pruebas/atlas-siguiente-20261105/scene081/`.
