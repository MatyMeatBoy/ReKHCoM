# Escena097: bordes de árboles y tronco

La capa BG2 de árboles se estaba limitando al rectángulo de repetición del
césped: x64..383/y0..319. La ROM contiene391píxeles opacos adicionales de hojas
laterales y tronco inferior (112izquierda,135derecha,144abajo). Conservar el
mapa BG2 finito completo recupera esas partes sin repetir árboles/props.
El césped BG3 sigue usando exactamente su patrón aceptado.

Respuesta sólo a la nube7f9b6ee0 del parentec8c60b820a847aeabe9778e8dce033d,
revisión3. Las otras dos nubes relativas a desaparición/aparición temporal del
gato siguen abiertas. El modelo sin actores no prueba el momento del evento.
No se altera el código actual de staging de Cheshire, sombras ni viewportGBA.

18.818casos AST del selector C real pasan; nuevas coordenadas BG2 son identidad
finita dentro de64x64tiles y fuera devuelven vacío. BG3 idéntico. El estudio
ROM antes/propuesta conserva encuadre/escala y no es captura nueva. Fixture C
añadido, sin compilar ni ejecutar; falta QA real de la cámara/actores/Original.
Sin build, juego, imports, ROM/SAV alterados ni paquetes.
Evidencia: `pruebas/atlas-20261106/scene097-tree/`.
