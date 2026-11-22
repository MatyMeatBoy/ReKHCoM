# Escena057: unión de madera sobre la enredadera, estudio sin build

Padre Atlas `ee7aa74a63ef4b1e905f64828190f38d`, revisión2, nube
`b9d8aa2c-e247-476b-8157-c87f7dd31cac`, «mejorar». Imagen SHA256
`5971f1bf95f3e4d5a4d8ffe3e35eabe08b924f5acb46fa569f8709163e080132`.
Su receta anterior está integrada en PR599; la nueva propuesta no hereda aprobación.

Interpretación de la zona señalada: una copia parcial de ventana/cornisa aparece
por encima de la enredadera única, interrumpiendo la madera. Se propone retirar
sólo BG2 de tiles x20..27/y-8..-1, exponiendo los tablones completos de BG3.
La enredadera original empieza en y0 y conserva todos sus píxeles; tampoco
cambian puerta, ventanas derechas desde x28, palmera, matas o suelo aceptados.
El selector se usa fuera del cuadro GBA. Sólo selecciona el tile transparente
original(0,0) en BG2: no hay paleta o bitmap nuevo.

Verificación sin build: regresión RED→GREEN interpretando el AST del selector
C real.24.576coordenadas coinciden con el modelo y sólo28selecciones cambian,
todas dentro del rectángulo indicado. Todas las filas nativas y>=0 permanecen
idénticas. El recorte completo del antes coincide píxel a píxel con la captura
histórica057/600, calibración3x/origen(579,171) y expansión RGB555.

La comparativa nueva se rotula **estudio/simulación sin build**; no es nueva
captura del juego. El fixture C se amplía pero no se ejecuta. No compilador,
DLL, juego, importación o paquete. Otras cámaras, Original ejecutado, nuevas
plataformas y valoración de la hija siguen pendientes.
Evidencia privada: `pruebas/atlas-siguiente-20261105/scene057/`.
