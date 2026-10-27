# Indicaciones de mando experimentales — etiquetas, símbolos y preset

Pedido del usuario para 2.1; carril `controller-prompt-layouts-v21-experimental`,
owner `codex-control-prompts`. La opción de Controles **Indicaciones
(experimental)** persiste `controller_prompts`: 0 Original/GBA (predeterminado),
1 Auto, 2 Nintendo, 3 Xbox, 4 PlayStation. Es una familia visual; no cambia
bindings. Restaurar controles predeterminados también restaura Original/GBA.

## Primera integración textual (#332)

- Resolver acción → binding efectivo → posición física → nombre de familia.
  Deshace la normalización Nintendo existente antes de nombrar la posición.
  PlayStation muestra Cross con A ligado a South; sólo muestra Circle si A
  está realmente ligado a East. No se deduce confirmación de la región.
- Auto usa el tipo SDL del último mando con actividad. Desconocido usa nombres
  de posición; no inventa Xbox. Teclado activo o mando desconectado usa la tecla
  efectiva, con alternativa secundaria si la primaria está vacía.
- Se registra actividad cruda antes de que el menú consuma el evento o Nintendo
  normalice sus botones. Un eje debe cruzar 16.000; ruido menor no cambia la
  familia. Sólo se guarda un ID, se vuelve a consultar SDL y no se retienen
  punteros de mandos desconectados. No carga imágenes ni enumera mandos por
  cuadro; los nombres se resuelven al consultarlos, sin caché obsoleta.
- Controles y los indicadores A/B que usan `MenuButton` reutilizan el diseño
  existente con etiquetas textuales. Los tutoriales EU/US resuelven únicamente
  spans resaltados exactos de botones dentro del rango de mensajes tutoriales.
  Se conservan prosa, nombres y controles de texto. Los templates observados
  incluyen `A Button`, `A-Knopf`, `Bouton A`, `Pulsante A`, `Botón A` y L/R.
  Las etiquetas nuevas usan corchetes y el motor existente de texto/layout.
- Conserva la inversión de scroll ya existente antes de resolver L/R. Un
  buffer acotado independiente conserva la fuente original ante exceso de
  longitud. No reemplaza letras globalmente ni modifica ROM/SRAM.

## Segundo incremento: símbolos y tokens SJIS

- Base programática pequeña: letras 3×5 del diagrama existente, cuerpo de
  botón 12×10 y primitivas Cross/Circle/Square/Triangle. No depende de que la
  fuente del host incluya esos símbolos, ni añade arte externo. Original/GBA
  conserva sus indicadores existentes y su comportamiento predeterminado.
- Caché inmutable de 18.432 bytes de tiles 4bpp + 144 flags. Cada símbolo se
  genera una vez; su dirección no cambia. `LoadObjTiles` reutiliza esas fuentes
  estables. No se cargan imágenes ni se asigna un atlas por cuadro.
- El resolver emite un token privado de dos unidades sólo para acciones
  reconocidas. Los layouts reales Latin/SJIS lo consumen como un sprite y
  conservan posición, paleta y avance de 12 píxeles. El tag 26 queda como icono
  pixel incluso con texto HD. Teclado y bindings sin símbolo conservan nombres
  textuales; en JP se codifican explícitamente como caracteres SJIS de ancho
  completo, nunca se copian bytes UTF-8.
- Patrones JP comprobados en datos locales: Ａ/Ｂ/Ｌ/Ｒ + ボタン, Ｌ・Ｒボタン
  y ＳＥＬＥＣＴボタン. Se conserva el sufijo japonés y la prosa ajena.
  Las combinaciones Latin conocidas L+R generan dos símbolos/etiquetas.
  Tutoriales de warp, escaleras y Riku tienen whitelist explícita adicional.
- Los layouts tienen un límite PC de 128 slots y continúan en otra página
  antes de escribir fuera del arreglo, incluso con expansiones de etiquetas.
- Indicadores de opciones `GbaKey`/`MenuButton` reutilizan estos símbolos cuando
  hay un mando conocido activo y mantienen fallback textual/Original.

## Preset explícito y confirmado

La fila **Preset confirmar/cancelar...** abre una pregunta localizada con
**No** seleccionado. Explica que reemplaza A/B personalizados del mando,
conserva teclado y todos los demás controles. Nintendo elige posiciones A/B;
PlayStation Circle/Cross; Xbox A/B. La familia elegida queda fijada al abrir la
pregunta: cambiar de dispositivo al confirmarla no elige otro preset. La
normalización Nintendo se deriva del input real/configuración de layout, nunca
de la familia visual. Auto desconocido y Original no ofrecen un preset inventado.

Pruebas: `test_controller_prompts.py` (contrato, patrones y raster 4bpp acotado),
`test_controller_prompt_layout.py` (productor real de tiles y consumidores
Latin/SJIS reales, dos símbolos, posiciones, fuente intacta y límite 128),
`test_card_scroll.py --version eu/us/jp` (input/config reales y SDL virtual:
INI, Cross/Circle, Nintendo, Auto desconocido, teclado/ruido/desconexión,
tokens JP y preservación de custom bindings fuera del preset confirmado).
Sintaxis de UI/mensajes EU/US/JP y `check_sources.py` pasan. No equivale a
una build completa o revisión visual GPU.

## Pendientes obligatorios del alcance completo 2.1

- Ampliar sólo patrones de tutorial comprobados que falten; frases compuestas
  no reconocidas conservan el original. Revisar visualmente símbolos de todas
  las familias en pixel/HD y paletas EU/US/JP, sin afirmar cobertura total sólo
  porque un parser reconoce un nombre.
- Adaptar líneas ya abiertas al cambiar dispositivo/binding; ahora los mensajes
  cambian al volver a resolver su página. Revisar HD/pixel text, ancho, recorte,
  fuentes de símbolos, fallback accesible y mandos múltiples en pruebas GPU.
- La identificación de familia en guests depende del tipo que exponga su shim;
  hoy un tipo genérico conserva el fallback de posición. No afirmar detección
  universal ni paridad visual sin capturas de cada runtime/dispositivo.

Original/GBA permanece apagado por defecto y no cambia el texto original.
No hay porcentaje de cobertura ni promesa de soporte completo por esta entrega.
