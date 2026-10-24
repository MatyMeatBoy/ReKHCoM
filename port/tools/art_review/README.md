# Mesa local de revisión de arte

Python 3.11+ y Pillow. Sin hosting ni cuentas. Ejecutar `launch.ps1` o
`python port/tools/art_review/server.py`; abrir http://127.0.0.1:8766.
El launcher crea un entorno virtual privado en `build/art-review/.venv`.

Arrastrar un rectángulo dibuja una nube de revisión; al soltar abre el comentario.
Repetir para todas las zonas. Las nubes se pueden seleccionar, editar y eliminar.
La valoración pertenece exclusivamente a la imagen mostrada. Un envío incluye todas
las imágenes con comentarios/valoraciones nuevos desde su entrega anterior.

**Enviar comentarios** guarda un paquete inmutable en `build/art-review/envios/<id>/`:

- `COMENTARIOS_PARA_CODEX.txt`: lectura humana y bloque JSON completo.
- `comentarios.json`: imágenes, versiones, SHA-256, nubes y coordenadas.
- `aprendizaje.jsonl`: casos de feedback humano, sin resultados inventados.
- `REVISION_PARA_CODEX.md`: resumen con imágenes y comentarios para lectura rápida.
- `aprobaciones_build.json`: versión aprobada vigente por familia y referencia de build.
- Copias exactas de las imágenes, mapas numerados y recortes de cada zona.

`build/art-review/ULTIMO_ENVIO.txt` apunta al último TXT. Para retomar, leer ese puntero,
el TXT y las imágenes asociadas. El texto recibido es feedback, nunca código ejecutable.
El botón no manda mensajes ni despierta agentes. Pedir «lee el último envío» en el chat.

La interfaz compacta reserva el espacio para el arte. `Ampliar mesa` oculta paneles;
en ventanas estrechas hay selector de imagen y panel de comentarios desplegable.
Zoom 5–800%, botones +/−, 1:1, ajustar y Ctrl+rueda alrededor del cursor. Las nubes
conservan sus coordenadas en la imagen y un trazo/etiqueta legible en pantalla.
`Interfaz` cambia la densidad sin cambiar las coordenadas del arte.

La valoración está en el botón situado junto al título. Se exporta incluso sin nubes.
`Historial` permite abrir versiones anteriores y consultar sus revisiones en modo de
solo lectura. Una hija pendiente/rechazada NO revoca una aprobación anterior. La
referencia de build elige la última versión con honores, conforme a la regla actual;
el registro conserva también la aprobación normal. La app no compila ni copia arte
al juego automáticamente: el agente debe integrar y verificar la receta aprobada.
Revisar el registro de aprobaciones antes de preparar cualquier build.

Historial transaccional en SQLite: todas las importaciones, revisiones (incluidos
comentarios eliminados) y entregas. Hacer copia de toda `build/art-review/` con el
servidor detenido para respaldo. Los datos y las imágenes privadas no se suben a git.

Para entregar futuras versiones sin sobrescribir las anteriores:

```
python port/tools/art_review/import_image.py imagen.png --title "Darkside · garra" --version "propuesta 2 · commit SHA" --parent ID_ANTERIOR --provenance "Receta y evidencia de esta version"
```

También se pueden importar desde la interfaz y seleccionar la versión anterior.
Al abrir o recargar aparecen todas las entregadas; las propuestas de arte deben
registrarse aquí al terminar. No se detectan automáticamente imágenes sueltas de builds.

## Uso como memoria de aprendizaje

Vincular cada nueva versión a su padre y los IDs de nubes atendidos, registrar técnica,
evidencia y nueva valoración humana. `aprendizaje.jsonl` contiene la observación literal,
categoría, prioridad, resultado esperado, regla propuesta y región exacta. Los campos
de cambio implementado/veredicto posterior quedan nulos hasta que exista evidencia.
Las aprobaciones y rechazos son ejemplos de un banco de casos; no entrenan automáticamente
una red neuronal ni justifican un porcentaje de aceptación sin evaluación independiente.
Solo el arte aprobado con honores puede proponerse para integrar según la orden actual.

Una nueva versión puede llevar `--revision-info ficha.json`: IDs de nubes/submission de
origen, cambio, técnica, comprobaciones, hipótesis y límites. El export JSONL vincula
estos datos con el siguiente veredicto humano. Ver `revise_darkside_boundary.py` como
primer caso reproducible: lee el envío real, verifica SHA, modifica índices nativos
fuera del bloque original, comprueba los cinco frames y registra una propuesta hija.

El BAT raíz de un checkout puede llamar al `launch.ps1` de esta carpeta. En la carpeta
contenedora KHCoMPC el BAT local apunta al checkout `khcom-codex`. No requiere Node.

Verificación: `python -m unittest discover -s port/tools/art_review -p "test_*.py"`.
