# Escena120: plano derecho, corrección05/11

Padre exacto `e50805fc25d04bcda7a259b2fee2848b`, envío `20261105-120135-20af7d`, revisión8, general «falta la diagonal aun» y cuatro trazos. Su estado aprobado conserva feedback abierto y no aprueba esta nueva hija.

La geometría nativa de BG2 contiene dos zócalos: frontal x32/48/64/80 inicia en y88/80/72/64 (`2y+x=208`); derecho x112/128/160/192 inicia en y32/24/8/0 (`2y+x=176`). Extender todo desde64..79 desplazaba el derecho16px hacia abajo. Sólo fuera del mapa256x256, el plano derecho toma128..143 en la banda176..208; BG3 continúa mampostería hasta176. Frontal208 y mapa nativo intactos. No se copian lámparas, rótulos ni barriles.

El ajuste anterior de OBJ sigue en este PR: barriles decorativos completos, Pepito con rol estable4 y Hook3 conservado al rebasar main. Las sombras comunes no desbloquean al actor que espera. Los tests reales de ambos roles pasan.

Validación: `python port/tools/test_scene120_diagonal.py` RED sobre el donante anterior y GREEN después. Incluye salida por borde superior, mapa completo excluido y ambos planos. `test_scene120_staging.py` y `test_scene118_staging.py` GREEN.

Dos previews EU compilan sólo port_ppu.o contra el mismo resto de356objetos release en lectura: antes/solución usan la misma fuente PPU actual y difieren sólo en el header de la diagonal. No es una recompilación completa de main. Cuatro procesos seriales, modo Original y Extended antes/después, cuadros180/300/600/900,1920x1080,escala3, exit0. Las cuatro imágenes Original coinciden por completo; las cuatro Extended conservan el núcleo GBA completo. Diferencia limitada al muro derecho superior. La inspección real del cuadro900 confirma continuidad sin el salto16px junto a los barriles.

Pruebas privadas en `pruebas/atlas-final-20261105/SCENE120-DIAGONAL-RUNTIME-CHECKS.json`, `SCENE120-DIAGONAL-SOURCE-PROVENANCE.json`, `scene120-diagonal-RED.log` y los tres GREEN. Estudios ROM y nativos son internos. Nueva entrega ANTES/PROPUESTA del mismo cuadro900/escala3, pendiente de valoración e integración, sin heredar aprobación.
