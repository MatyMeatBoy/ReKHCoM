# Artes aprobados para una futura pre de 2.8.5

05/11/2026. Integración de fuentes autorizada; **sin compilar, ejecutar el juego,
regenerar imports ni publicar paquetes**. La versión pública y los paquetes v2.8
no cambian. Esta preparación no certifica un ejecutable combinado.

Se acepta la selección exacta `build_version` de Atlas, incluidos aprobados con
comentarios. Ninguna aprobación se hereda a una futura solución.

| Pieza | PR original | Imagen aprobada | Revisión | Seguimiento |
|---|---|---|---:|---|
| Escena 115 | #576 | `1bf2e47af7fa4b2fa02599248052a27a` | 4 | tile flotando rojo; falta punta; sobras de escotilla |
| Escena 095 | #580 | `ab3444223b854341afa9ae521a0d2860` | 5 | patron A; completar resto con patron A; completar resto con patron A |
| Escena 120 | #584 | `9f28568fabd94b7c8526615a7b2b6abf` | 5 | diagonal ok, completar grabado; A; completar con patron A |
| Escena057: madera superior y matas completas | #585 | `ee7aa74a63ef4b1e905f64828190f38d` | 2 | mejorar |
| Escena 130 · Bastión Hueco · Biblioteca de Bella | #587 | `dbe9d6b92ba846cdad63111c28b3f3d8` | 3 | falta completar diagonal de bajada de muro |
| Escena 110 | #590 | `e0826ddfad754b44952ae7b386e86f9c` | 1 | Sin comentarios nuevos |
| Escena 087 | #591 | `31800ebcfc224078bb623ab2b5f846a8` | 2 | mejorar pero el resto ok |
| Escena 081 | #592 | `0bac2ac10b81462e910b3705bd3c6740` | 2 | falta tapar con negro |
| Oogie Boogie · arena | #593 | `f3a2a2d29bdd45b5b040a9cca851edb4` | 5 | falta dibujar punta del fantasma solamente; falta completar mitad de la mascara solameente; completar hasta derecha; completar hasta izquierda |
| Escena 112 | #594 | `51bde4b6b4774c4291c8f50f4e83d8b1` | 1 | Sin comentarios nuevos |
| Sintesis: requisitos dentro de la carta y digitos del mod HD | #595 | `8fc06e77f2494657ae11c5437d1e1845` | 1 | Sin comentarios nuevos |
| Agrabah 107 / 108 | #597 | `7e56559f87b1402da2fe92e2eafe256c` | 4 | falto poquito de la diagonal solamnete; falta completar ventana y patron hacia arriba de muro en generla; completar patron de muro hacia bien arriba, en esta escena luego se ve la parte de arriba |

## Qué conserva la integración

Las recetas y máscaras aprobadas se conservan exactamente. El conflicto común
de actores reúne las políticas existentes: Cheshire0, Aladdín1, Genio2, Garfio3,
Pepito4 y Maléfica5. Las sombras no desbloquean actores. Las entradas del dueño
de sombra de112 se limitan a2/3; no se interpretan bytes antiguos como índices.
La extensión130 pasa a la lista normal de aceptadas; conserva el apagado explícito
`KHCOM_SCENE130_WIDE=0` y la ruta Original.

Síntesis cambia405recetas de importación. Su regeneración queda para una futura
pasada autorizada; los imports personales actuales no se reescriben aquí.

## Verificación sin build

- SHA256 de las12 imágenes servidas y revisión/selección de Atlas comprobados.
- Recetas individuales preservadas por comparación de fuentes y hashes.
- Las políticas combinadas de actores se interpretaron desde su AST:17.920casos
  de elegibilidad/rol iguales a sus ramas originales en97/112/118/120/130.
- Pruebas Python: biblioteca1 y síntesis4. Sin invocar compiladores.
- Fixtures C actualizados para los símbolos comunes; compilación y runtime
  combinado quedan pendientes por instrucción expresa.
- Pruebas/capturas previas por arte se conservan como evidencia histórica.

## Pendientes vigentes

Nueve entregas integradas conservan sus nuevos comentarios sin responder. Se
registran como tareas nuevas, sin apropiarse de los WIP anteriores. Atlas conserva
todos sus envíos, nubes y trazos; integrar no equivale a responderlos. El resto
del feedback anterior (Darkside, Diario, Maléfica, START, Monstro, cabecera de
síntesis y Ver barajas) sigue pendiente. Brick Launcher queda fuera de este proyecto.
Las referencias nativas son internas; las arenas ya existen.

Las menciones a propuesta pendiente en las fichas originales son históricas y
quedan supersedidas sólo respecto de integración por esta tabla. Los comentarios
aún abiertos sí siguen vigentes. Evidencia privada:
`pruebas/atlas-integracion-20261105/`.
