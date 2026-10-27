# Síntesis HD: receta actual y cobertura completa

Base de trabajo: main `8d495811e`, familia de recetas de PR #371,
`FORM/0102/FO0102.RTM:UK_room01.imz#0`. No se usa el generador Pillow
legacy ni su staging de 128×156. Los cuerpos importados actuales miden
256×312; son cinco cuerpos y 400 requisitos (cuatro reglas por 0..99).

## Verificación del consumidor

`test_synthesis_import_coverage.py --texture <UK_room01.png privado>
--work <evidencia privada> --baseline-sprites <sprites actuales>` crea un
subdirectorio nuevo. Extrae sin modificar `Rgba`, `Resample`, `BlitOver`,
`Ink`, `PolyFill`, `ApplyRecipe` y sus helpers de `hd_import.inc`, compila
sólo ese consumidor C++ y ejecuta las 405 recetas. No requiere SDL ni juego.

Comprueba dimensiones, alpha sin cortes en los bordes, nombres únicos,
paridad con el consumidor Python (diferencia máxima de un nivel por canal),
y que los 400 requisitos coincidan píxel a píxel con la importación actual.
La textura privada se verifica por SHA-256 antes y después. Las 20 planchas
componen las 2.000 combinaciones a sus orígenes OAM; son planchas de recursos,
no capturas del juego ni prueba de filtrado GPU.

El arnés C existente `test_synthesis_requirement_hd_logic.c` comprueba las
2.000 selecciones del adaptador y ahora también las 2.000 inicializaciones
reales `InitEventKeyCard`→adaptador, además de los controles de HD apagado,
recurso ausente, carta específica y TOTAL10→pago1→actualización9→pago9.
100 mantiene el fallback original: no se añade una identidad de tres cifras
ni se cambia la regla o el saldo del juego.

## Acabado inferior: experimento retirado, pendiente

El bisel de room01 suma alpha sobre el cristal al terminar: en el centro del
cuerpo, alpha89 sube hasta158 antes de caer. La propuesta atenúa sólo los
últimos siete texels del bisel mediante `erase`, `blit` y `fade` existentes.
No añadía operaciones al importador. Los píxeles superiores a y287 permanecían
idénticos a la base; se verificaron los cinco cuerpos. Los dígitos y operadores
no cambiaban. Esta propuesta NO se incorpora a las recetas de la rama: queda
archivada en `pruebas/synthesis-gray-digits/recipe-proposal-unapproved.patch`.

Una franja anterior observada en GPU también coincide con dos tramos OAM
solapados del cuerpo (y162..306 y y302..338 en la captura1280×720). La
atenuación inferior no resuelve ese solapamiento. La franja señalada por el
usuario probablemente coincide con esa región; hay que validar el doble
blending de los dos tramos antes de atribuirla sólo al recurso. El pendiente
se devuelve a cola por prioridad del crash #386, sin afirmar acabado aprobado.

## Evidencia privada y límites

`pruebas/synthesis-gray-digits/room01-coverage-b4oru1y8/results.json` contiene
hashes del experimento retirado: textura, funciones reales, generador, recetas, ejecutable de prueba
y405 PNG. Fuente UK privada SHA-256
`e7030c6fe0c065abec27bda3eb916062b37f1ceac9fc0cb01bce5d56a9d803d4`.
La receta sin cambios reproduce exactamente el cuerpo actual antes de refinar.

`pruebas/synthesis-gray-digits/gpu.json` registra dos ejecuciones A/B de la
misma escena de puerta normal con Direct3D11 real, ventana oculta y audio
dummy. Se usa el ejecutable EU32 existente SHA-256
`4bb9b994dc91790cbf7f24940dfb21c9d2f690626a44abb26d7b58afb27dac1a`,
sin recompilar producto. Sólo cambian los cinco cuerpos en fixtures privados;
el resto del conjunto HD se conserva. `gpu-before-after.png` es un recorte
comparativo de esas capturas y `gpu-{before,after}/current.png` las completas.

Pasan importación405, paridad, inicialización2000, selección2000 y ambos
runs GPU con rc0. La captura GPU cubre una puerta; no se afirma una ejecución
GPU de las 2.000 combinaciones ni una mejora de rendimiento. No se toca la
instalación del jugador, fuentes privadas, guardados reales ni recursos del
guante de otro carril. Los PNG privados de Re:CoM no se incluyen en el PR.
