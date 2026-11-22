# Atlas: no ocultar los envíos respondidos parcialmente

La cola trataba cualquier hija posterior como respuesta completa al envío del
padre, incluso cuando `revision_info.response_to` nombraba sólo una de sus nubes
o era una lista vacía. Eso retiraba el trabajo pendiente del turno del agente.

Ahora el fallback por antigüedad de hija se limita a versiones antiguas que no
contienen la clave `response_to`. Si la clave existe, sólo cuentan las respuestas
explícitas existentes: una lista parcial o vacía deja el envío sin responder;
la lista completa lo cierra. No cambia el payload del envío, su contador, el
tratamiento de comentarios generales ni los criterios existentes de identidad.
No se modifica ninguna valoración o envío guardado.

TDD Python/Pillow: padre con dos nubes y cuatro casos de hija. Antes del fix:
dos fallos esperados para respuestas parcial/vacía; legacy y completa pasaban.
Después: suite `python -m unittest test_review`, 16 pruebas PASS. Las pruebas
usan directorios temporales y sus fixtures HTTP efímeros; no se ejecuta el
servidor persistente, el juego ni un compilador. `git diff --check` PASS.

Evidencia privada: `pruebas/atlas-siguiente-20261105/queue-partial/`.
