# Escena095: completar el árbol de la costura izquierda

Seguimiento del comentario general «verificar que el arbol del inicio de la
escena no este solo su mitad», Atlas `e78f689cba2e46548b67cff57c10f444`,
revisión2, SHA256 `08f5ea9ea3065e3f7770c41c1bcb82aea4c3ac0a48e6071036195c7393063aa7`,
envío `20261105-204012-31a5b7`. Su patrón A aceptado permanece intacto.

La ROM EU local contiene la copa inferior del topiario hasta375 y el tronco
hasta382. La receta BG1 refleja x0..15 hacia x=-16..-1, pero sólo hasta351.
Se amplía su límite exclusivo a384:248píxeles opacos originales completan
la mitad inferior, sin añadir objetos ni imágenes al juego. Las demás capas
y coordenadas conservan exactamente su selección anterior.

El caller de `PortPreliminarPixelSource` conserva la condición exterior al
viewport GBA. No se cambia el núcleo240x160, el modo Original, la composición
de actores, las paletas activas ni las recetas de muro/remate/patrón A.

Verificación: fallo previo de la prueba Python en(-16,352), seguido de
39260casos de AST de la rama C real;248píxeles nuevos, contenidos en
x[-14,0), y[352,383). Modelo antes/propuesta de la misma costura y escala4,
con fondo y primer plano decodificados de la ROM. Las referencias nativas
se guardan internamente; la imagen de revisión presenta la receta aplicada.

**SIN BUILD:** el estudio no es una captura del juego ni certifica cámara,
actores u oclusión en ejecución. Pendiente QA visual actual cuando se
autoricen compilaciones. No se ejecutó compilador/juego, no se regeneraron
imports y no se modificaron ROMs, partidas, instalaciones o paquetes.
La hija pending no hereda la valoración de su padre.

Evidencia privada: `pruebas/atlas-20261106/scene095-tree/`.
