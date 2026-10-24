# Ficha de revisión de un fondo — copiar por pieza

Seguir [WORKFLOW_ARTE_AMPLIO.md](WORKFLOW_ARTE_AMPLIO.md). Dejar vacía una
casilla sin evidencia; la revisión del agente no es aprobación del usuario.

## P0/P1 — identidad y base

- Símbolo/fondo; escenas, eventos y combates compartidos:
- Captura/comentario identificado por contenido:
- Dimensiones de almacenamiento, bounds dibujados, hashes/capas por región:
- Base elegida: commit, ejecutable, capturas, autor/modelo conocido:
- Por qué es la versión más avanzada validada; borradores descartados:
- Referencias aprobadas abiertas; un borde y un objeto observados en ellas:
- Estado inicial y comentario del usuario que obliga a esta revisión:
- Directorio privado del estudio y exportaciones de capas:

| Zona/capa | Geometría original | Defecto actual | Evidencia/crop | Acción |
|---|---|---|---|---|
| Izquierda | | | | |
| Derecha | | | | |
| Arriba | | | | |
| Abajo/esquinas | | | | |

## P2 — planos medidos

Unidad: píxeles del fondo / tiles 8×8. No mezclarlas.

| Límite/plano | Capa | Punto A | Punto B | Ecuación, intervalo, fase | Grosor/remates | Dos puntos exteriores esperados |
|---|---|---|---|---|---|---|
| Pared A/suelo | | | | | | |
| Pared B/suelo | | | | | | |
| Retorno/otros | | | | | | |

- [ ] Cada diagonal conserva pendiente, intercepto y ambas paridades.
- [ ] No hay esquinas nuevas; la perspectiva frontal, si existe, se trata aparte.
- Ruta del esquema de planos:

## P3 — donantes y objetos

| Uso | Capa/recurso | Rectángulo píxeles | Rectángulo tiles | Período X/Y | Transformación | Excluye | Alternativa fuera de rango |
|---|---|---|---|---|---|---|---|
| Pared | | | | | | | |
| Friso/cornisa | | | | | | | |
| Suelo/otros | | | | | | | |

- Objetos únicos y piezas cortadas por el borde: posición y cómo completarlas:
- Vacíos intencionados y transparencia: qué superficie se ve debajo:
- Decoración repetible: recurso completo, anclaje y separaciones:
- Pixel art mínimo, si hace falta: recurso imitado y por qué no basta copiar:
- [ ] Donantes íntegros: sin celdas vacías inesperadas ni trozos de objetos.
- [ ] Textura con módulo completo; remates y objetos clasificados por región.

## P4/P5 — implementación y revisión visual

- Archivos/funciones, selector de activación y desactivación individual:
- Ejecutable (SHA256), commit o diff privado de esta iteración:

| Imagen abierta | Ventana/estado o crop `(x0,y0,x1,y1)` | Qué se comprobó | Veredicto/defecto pendiente |
|---|---|---|---|
| Panorama 3440 | | Planos/silueta | |
| Panorama 3840 | | Extremos | |
| Unión izquierda | | Borde/friso/sombra, dos paridades | |
| Unión derecha | | | |
| Unión superior | | Columna/relieves que cruzan y=0 | |
| Unión inferior/esquina | | | |
| Referencia aprobada | | Continuidad y densidad | |

- [ ] Revisión a 1:1 y x3 sin filtro; no solo miniatura.
- [ ] No se delata un rectángulo pegado; cada línea llega continua al extremo.
- [ ] Objetos completos, sin clones; decoraciones con contacto al suelo.
- Revisor/modelo real, fecha y decisión: avanzar / volver a paso___ por___ .

## P6 — comprobación técnica real

| Fixture | Región | Amplia: ventanas/estados | Original | Fases realmente vistas | Informe/resultado |
|---|---|---|---|---|---|
| | EU | | | | |
| | US | | | | |
| | JP | | | | |

- Controles de aprobados/anteriores y resultado:
- Cobertura no realizada e intentos inválidos:
- [ ] Informes del ejecutable posterior al último cambio.
- [ ] `native_diff:null` es igualdad; `passed/changed/READY` no certifican arte.

## P7 — cierre y siguiente base

- [ ] Cero defectos estructurales o comentarios de esta pieza sin responder.
- [ ] Evidencia final preservada; copia jugable actualizada cuando corresponda.
- [ ] PRELIMINAR_V1 actualizado; commit de esta pieza:
- Base validada para continuar: commit, evidencia, modelo/revisor:
- Estado: propuesta corregida/comprobada pendiente del usuario / aprobación explícita.
- Aprobación real (dejar vacío si no existe): cita, fecha y versión/captura exacta.
- Próxima pieza y pendientes independientes conservados:
