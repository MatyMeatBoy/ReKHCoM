# Escena120: completar el grabado figurativo

Respuesta separada a la nube65a0a5d0 de Atlas9f28568fabd94b7c8526615a7b2b6abf,
revisión5. La mamposteríaA de PR601 continúa pendiente y no se incluye aquí.

Los dos grabados BG2 están truncados por el borde superior del mapa, pero sus
tiles superiores sí existen en el banco de960tiles que el fondo carga. La fila
visible comienza con692..699; las columnas20..27 de las filas15..20 contienen
sus cabezas y brazos. La continuación usa x96..159/wy[-48,0), con la primera
figura desde-32, paleta4 e índices de tinta8/10/11/12. No se copian los fragmentos
dorados ajenos ni el fondo de ladrillos del atlas de gráficos.

El descriptor sólo devuelve esos tiles ya cargados. Su lector4bpp comprueba el
límite VRAM y toma la paleta viva. Se llama sólo en escena120/BG2 dentro de la
ruta existente exterior al viewportGBA. Ningún wy>=0 ni objeto, actor, plano,
mampostería o receta de importación cambia. No se añaden raster, ROM o assets.

La hija pendiente muestra antes/modelo al mismo encuadre y escala, sin actores
ni captura nueva. No hereda aprobación.20.480casos AST del descriptor/lector con
ROM, cuatro bases de caracteres y guardas de memoria;696píxeles de tinta nuevos.
Fixture C añadido para futura verificación, sin compilar ni ejecutar.

Sin build, juego, imports o paquetes. Faltan prueba real de cámara, prioridades,
GBA/Original y plataformas. Evidencia: `pruebas/atlas-20261106/scene120-figure/`.
