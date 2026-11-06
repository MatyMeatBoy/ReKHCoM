# Escena081: completar los dos bordes del patrón

Las dos nubes de Atlas9224a996d76947d48204e2501ede8e80 revisión3 piden
mejorar el patrón del borde superior e inferior. El módulo BG1 de64px
x128..191 salta unas14filas entre su final e inicio. Desdewx192 se repite
su segunda mitad160..191, un ciclo diagonal completo de32px; las muestras
conservan2sy+sx=2wy+wx. Se mantiene el remate finito y el primer módulo.

El nuevo borde descubre celdas transparentes de BG3 que la máscara anterior
ocultaba. Sólo esas1024posiciones de donante superior se completan con el
tile original16filas debajo, comprobado opaco en ROM. Las muestras del piso
opaco, los props BG2, los actores y la omisión decorativa aceptada no cambian.
El muestreador exterior de tiles usa la misma receta de4tiles/2filas.

La hija de Atlas muestra ANTES/PROPUESTA del modelo de receta, con los mismos
tiles, encuadre y escala; no es captura del juego ni referencia nativa aislada.
No hereda la aprobación del padre. El ensayo de superponer cambios sobre una
captura histórica no se entrega: no coincidían todos sus píxeles de fondo.

69.222coordenadas del C real interpretadas desde AST y10.146casos de retícula
de tiles coinciden con el modelo;
cambios limitados aBG1/BG3 después de192, fase exacta del borde, piso opaco
intacto. Se verifica la transparencia de las1024posiciones y la opacidad de
sus donantes. Fixture C actualizado, sin compilar ni ejecutar. Faltan cámara,
GBA/Original y compositor real del ejecutable nuevo. Sin imports ni paquetes.
Evidencia privada: `pruebas/atlas-20261106/scene081-patterns/`.
