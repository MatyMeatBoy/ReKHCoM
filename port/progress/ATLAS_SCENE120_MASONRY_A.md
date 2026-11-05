# Escena120: módulo A, estudio sin build

Seguimiento de Atlas `9f28568fabd94b7c8526615a7b2b6abf`, revisión5,
SHA256 `206dfb152f5ee54ba94e1e6c9796b228755fc2aeb465df112a99ab1efc37e050`,
envío `20261105-160839-40fd46`. Su diagonal está aceptada; la nueva hija
no hereda aprobación. Las nubes A y «completar con patrón A» piden que la
mampostería superior conserve los ladrillos completos de la franja nativa.

La receta16x16 repetía un fragmento de ladrillo. El módulo marcado tiene
espacio nativo limpio x32..95/y0..31. Usar ese bloque64x32 conserva juntas
de32px y variación de material, sólo en BG3 fuera del mapa y a la derecha de
la columna. Todos sus píxeles están antes de la diagonal176 (máximo157):
no contiene suelo, lámparas, cartel, figuras ni barriles.

La diagonal176/208, BG2 y BG1 se conservan exactamente. El grabado figurativo
se corta en el borde superior del mapa; **sigue pendiente**, no se responde
la nube «diagonal ok, completar grabado» con esta propuesta de mampostería.
Tampoco cambian actores, sombras o viewport nativo.

Por instrucción humana no se compila C ni se ejecuta el juego. La comparativa
antes/solución es un modelo Python con las capas originales de la ROM local,
misma cámara y escala, sin actores; no es una captura runtime. El modelo
comprueba65.536coordenadas nativas intactas y291.840coordenadas de cada capa
BG1/BG2 sin cambios. Se añadieron dos aserciones al fixture C existente para
la futura pasada autorizada, pero no se ejecutaron aquí.

Evidencia privada `pruebas/atlas-siguiente-20261105/scene120/`. Pendientes:
grabado figurativo, build y prueba real de la receta combinada, otras plataformas.
