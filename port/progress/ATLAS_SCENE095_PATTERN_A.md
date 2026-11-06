# Escena095: continuar el patrón A derecho — propuesta sin build

Nueva revisión humana: Atlas `ab3444223b854341afa9ae521a0d2860`, revisión5,
SHA256 `c2aa6e93995400df66c41e894a6865b8ac140cdfea470d8bd3c812b7e87978fd`,
envío `20261105-160609-47f012`. Marca el tramo oscuro contiguo al trono como
«patron A» y pide completar el resto derecho con él. La fuente aceptada anterior
ya está integrada en PR599; esta propuesta no hereda su aprobación.

El donante reflejado de la cara izquierda traía hojas claras y flores que no
están en A. Se reemplaza sólo la banda derecha `2*y-x=[-96,16)` por el tramo
completo original x448..479, conservando su propia diagonal y sombra. No cambia
el remate superior, la base, el muro izquierdo ni los objetos únicos. El PPU
sigue limitando el muestreo a los píxeles fuera del viewport GBA.

Se conserva la resolución de tiles8x8 y la paleta activa de la ROM local; no hay
recoloración ni bitmap nuevo en el repositorio. Las referencias ROM son internas.

La comparativa es una **simulación sobre una captura previa** del fotograma600,
no una ejecución nueva. Un parche nativo16x76 a escala3 calibra exactamente la
cámara en(-60,-60). Sólo se sustituyen píxeles que coinciden con el donante
anterior:88938píxeles físicos en9882coordenadas mundo; actores, primer plano,
diálogo y núcleo GBA se conservan. El nuevo tramo usa11entradas residentes.

Verificación sin compilador: interpretación del AST de las expresiones C reales,
87808coordenadas válidas con fase diagonal exacta y tiles cargados. ROM/símbolos
locales coinciden por SHA256 con la fuente del estudio. Fixture C actualizado
para una futura pasada autorizada, **no ejecutado**. `git diff --check` PASS.

Pendientes: compilar y verificar el resultado real, Original, otras cámaras y
plataformas cuando el usuario autorice builds. No se declara arte final ni se
ha cambiado instalación, ROM, partida o imports. Evidencia privada:
`pruebas/atlas-siguiente-20261105/scene095/`.
