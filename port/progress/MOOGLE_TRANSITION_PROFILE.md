# Entrada Moguri: clasificación única del fondo

La captura GPU del coordinador en `0022c58f1` todavía reproduce el borde
inferior cortado: frame 524, margen izquierdo `(74,8,0)` frente a la línea
nativa `(222,0,24)`. Los cambios #324/#325 corrigen respectivamente los
clasificadores vivos y el borde inferior de Estado, pero no bastan para Moguri.

`ModeInit` seguía seleccionando `PORT_SIDE_EDGE_FADE` para `gModeMsTop`,
mientras `PortMenuBackgroundOnly` y `PortMoogleMenu` ya lo consideraban menú.
No es evidencia de decodificación tardía. El fondo y la composición lateral
usaban perfiles distintos durante la animación legítima del panel.

`ModeUsesMenuBackground` centraliza la lista para entrada y composición viva.
La prueba extrae también la llamada real de `ModeInit`: antes falla con
`0xC0000409` al entrar en Moguri; después pasa y conserva los perfiles de
Estado/Barajas/mapa y los estilos distintos de Diario y mapa general.

La confirmación GPU posterior sigue siendo obligatoria antes de declarar
resuelto el efecto visual. Para una reentrada caliente real, salir a campo
antes de volver al menú; repetir el comando del mismo modo no demuestra una
transición de salida/entrada. Conservar capturas de cuadros vecinos y el log
de cambios de modo. No declarar cubiertos todos los submenús con este caso.
