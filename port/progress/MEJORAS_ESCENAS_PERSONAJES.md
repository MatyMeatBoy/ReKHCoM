# Recorte natural de personajes en escenas amplias

Al extender el fondo fuera del encuadre GBA, los actores de eventos que el
cartucho coloca fuera de sus 240×160 pueden entrar accidentalmente en la vista
panorámica. En `097` (Wonderland E2, Cheshire), se conserva el fondo extendido
y se recortan los sprites de escenario en el límite del cuadro GBA. Para `009`
(Traverse Town E4, Aerith), la BAT prueba una propuesta que mantiene el evento
dentro del cuadro GBA durante toda la secuencia; esto conserva la composición
de la desaparición y la animación original sin cortar retratos ni diálogos.
Este encuadre es optativo para revisión y no se activa en juego normal.

Ambas soluciones respetan la secuencia original de aparición, desaparición y
mezcla de color. La prueba no inventa un fundido para Aerith: permite revisar
el efecto del cartucho en el encuadre fijo propuesto. Otras escenas deben
revisarse individualmente antes de añadirlas a la lista.

`Probar Aerith - encuadre.bat` ejecuta el evento 009 a zoom 6 y guarda una
referencia GBA, la panorámica actual y la propuesta de encuadre nativo
en `build/scene-aerith-review`. Las capturas y `report.json` permiten revisar
los fotogramas alrededor de la entrada y salida de Aerith. La BAT usa
`KHCOM_AERITH_NATIVE_FRAME=on` para la pasada candidata.
