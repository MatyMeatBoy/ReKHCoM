# Base del indicador SELECT — ajuste de la pre-final

Pedido del usuario: alinear el final de la barra negra superior con la base de la luz, como PULSA START.

La lámpara HD ocupa once filas nativas y la barra del port usaba diez. El título HD usa ahora una base de once;
su texto baja la misma fila para conservar el espacio entre letras y subrayado. Las demás barras conservan diez
filas y el título sin HD conserva su geometría exacta.

Verificado:

- Prueba C de las funciones de producción: antes, base de lámpara HD 16 y barra 15; después, ambas 16.
- Captura real 1920×1080: antes, base de barra 107 y lámpara 113; después, ambas 114, incluyendo el píxel
  de relleno de la placa. El contenido de la lámpara no cambia.
- 1920×1080 y 1920×997: cambios limitados al cartel superior; todo lo situado por debajo conserva sus píxeles.
- Título sin HD 1920×1080: captura completa idéntica a la pre-final anterior.

Evidencia privada: `D:/01Claude/ReKHCoM-build/title-light-baseline-fix/runtime-report.json` y `title.png` por caso.
El ejecutable de comparación se enlazó aparte con los objetos EU de la pre-final 42995a75c, sustituyendo solo
port_ui.o. El árbol de compilación y el ZIP privado anterior se conservaron.
