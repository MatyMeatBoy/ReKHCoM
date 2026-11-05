# Oogie: propuesta de flancos con perspectiva, 05/11

Responde al padre Atlas `677876e57b1441dc856caa8847f84487`, envío `20261025-214551-252c9d`, revisión8: cuatro trazos para los límites blancos y verde igual a los bloques. La base existente ya es arte completo; esta propuesta corrige sus flancos.

Los planos arrancan junto al rail, ensanchándose con pendiente0.57 y ribete14+0.1·profundidad. Se calibró el encuadre de la captura original antes de medir los trazos. El verde es el índice159 de la paleta activa. Los contornos laterales y cierres inferiores de la máscara y fantasma se dibujaron independientemente; no se reflejan mitades ni se repiten figuras. La rampa blanca procede sólo del rail limpio de ROM. El vacío queda fuera de ambos bordes.

`Lines`, `PathIndex` y la exclusión del recinto nativo permanecen intactos. `oogie_motifs.py` reproduce las máscaras propias; no contiene datos de cartucho. Los índices157/158 suavizan únicamente el contorno nuevo con colores nativos.

Validación: `python port/tools/test_oogie_side_planes.py` falla con la fuente anterior en el primer punto verde y pasa con la propuesta. Comprueba exclusión del recinto, prioridad exacta del centro, flancos, contornos, ancho creciente, vacío y paleta en vivo con fixtures sin ROM.

Dos previews EU reutilizan los mismos356objetos de release en lectura: cambia sólo `port_oogie.o`. Cuatro procesos seriales Original/Extended antes/después, cuadros300/600,3840x1080,escala3, exit0. Original coincide en toda la imagen. Extended conserva todos los píxeles del recinto nativo y el pasillo rojo visible:52filas en300,60en600. La cámara sube24px en600 y el ROI se ajusta a esa traslación. No certifica otras plataformas ni una recompilación completa de main.

Evidencia privada: `pruebas/atlas-final-20261105/OOGIE-SOURCE-PROVENANCE.json`, `OOGIE-NATIVE-AND-PATH-CHECKS.json`, `OOGIE-FINAL-RUNTIME-CHECKS.json` y `oogie-final-runtime-*`. La entrega Atlas es ANTES/PROPUESTA runtime del mismo cuadro/escala, pendiente de valoración e integración; las referencias nativas y prototipos son internos.
