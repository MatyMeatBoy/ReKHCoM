# Biblioteca130: bajada diagonal del zócalo, estudio sin build

Seguimiento de `dbe9d6b92ba846cdad63111c28b3f3d8` revisión3,
envío `20261105-161054-5d8ad4`, nube384e2436 y trazo071834f9. El usuario
pidió continuar la base del muro hacia abajo, más allá del remate ya completo.

El zócalo ocupa592<=2*y+x<624. La receta anterior resolvía alfombra desde592,
por lo que perdía la banda decorativa al prolongarse a la izquierda del mapa.
El remate aprobado cubre x24..47; ahora x<24 resuelve esa banda antes de la
alfombra con un donante nativo64px de la misma diagonal. No se añaden colores
ni dibujos: se usa el zócalo completo de la ROM local.

El remate476píxeles conserva su rama y donantes exactos. Los138.848píxeles
autorados restantes conservan su fuente y las modificaciones del modelo quedan
limitadas a x<24 y a esa banda diagonal. No se modifica el PPU, la activación130,
la alfombra fuera de la banda, el escritorio o staging de Maléfica/sombras.

La comparativa es un modelo Python de BG2/BG3, misma cámara y escala, sin
actores y rotulada ESTUDIO SIN BUILD. No es captura runtime. Las aserciones
nuevas del fixture C quedan para la próxima pasada autorizada; no se ejecutó
ningún compilador, juego, importador o build. Pendientes valoración de esta
hija y prueba real del resultado combinado, sin aprobación heredada.

Evidencia privada: `pruebas/atlas-siguiente-20261105/scene130/`.
