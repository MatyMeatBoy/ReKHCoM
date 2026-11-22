# Oogie: máscara, punta y remates — propuesta sin build

Atlas `f3a2a2d29bdd45b5b040a9cca851edb4`, revisión5, SHA256
`ece4d12629f6ea98b8b88b56a09ce0043961fd7987ea5f6f4e5a4ccfc4bb32cd`,
envío `20261105-160839-40fd46`: completar media máscara, punta del fantasma y
los extremos de la franja superior. La arena ya existe; no se crea otra.
La versión previa aceptada se conserva en main60133ab69/PR599.

Se continúa la mejilla/mandíbula izquierda con un contorno independiente y
su hueco, unido al dibujo nativo. La punta exterior del fantasma recibe un
pequeño remate propio. Sin espejo de figuras, colores fijos ni copia de píxeles
del cartucho al repositorio: las máscaras usan los índices activos146/157/158.

Los extremos señalados son las ocho filas400..407 de la franja frontal. Esas
filas usan índices uniformes en sus tramos exteriores limpios (131/132/133);
la última conserva dos uniones al camino central dentro del cuarto. La receta previa
las cortaba al superar32píxeles fuera del cuarto. Ahora continúan hacia ambos
bordes con `RoomIndex(ROOM_X,y)`, manteniendo la paleta/fundido actuales. No se
cambia el cuarto original, `PathIndex`, perspectiva ni anchura de planos verdes.

La imagen nueva es **simulación sobre la captura previa300**, escala3,
transformación(3,792,-492). Los8383píxeles mundo sustituidos coinciden exactamente
con la receta anterior antes de simular (75447píxeles físicos). El cuarto entero
original es idéntico y el camino rojo no se sobreescribe. No es nuevo runtime.

La propuesta queda disponible con `KHCOM_OOGIE_PREVIEW=detail`. Sin esa opción
se usa la receta aprobada del PR593. El fixture C compara los resultados
del renderizador con las dos fuentes exactas sobre mapas y paletas sintéticos.
Las pruebas Python comprueban ambas máscaras y su generación. Falta QA en
el juego. La propuesta sigue pendiente de valoración. Evidencia privada:
`pruebas/atlas-siguiente-20261105/oogie/`.
