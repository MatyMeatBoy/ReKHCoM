# Indicaciones de mando experimentales — primera integración

Pedido del usuario para 2.1; carril `controller-prompt-layouts-v21-experimental`,
owner `codex-control-prompts`. La opción de Controles **Indicaciones
(experimental)** persiste `controller_prompts`: 0 Original/GBA (predeterminado),
1 Auto, 2 Nintendo, 3 Xbox, 4 PlayStation. Es una familia visual; no cambia
bindings. Restaurar controles predeterminados también restaura Original/GBA.

## Implementado en este incremento

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

`test_controller_prompts.py` valida el contrato puro. `test_card_scroll.py`
compila input/config reales y usa SDL virtual: INI, Cross/Circle con remapping,
normalización Nintendo, Auto desconocido, teclado, ruido, desconexión y tutorial
EU/US. EU/US/JP pasan; JP verifica que el texto SJIS permanece intacto, no que
ya tiene indicaciones adaptadas. También pasa sintaxis de `port_ui.c` EU y
`check_sources.py`. No se construyó ni ejecutó un juego completo.

## Pendientes obligatorios del alcance completo 2.1

- Símbolos gráficos pequeños para todas las familias y GBA/Original, reutilizar
  la base existente; esta entrega usa texto, no un atlas de símbolos nuevo.
- Tutoriales JP: tokens SJIS explícitos y glifos, sin introducir UTF-8 en el
  buffer del juego. Instrucciones compuestas L+R y otras frases todavía no
  reconocidas conservan su texto original; no afirmar cobertura total.
- Preset explícito de confirmación Nintendo A / PlayStation Circle, explicado
  y localizado. Nunca aplicar al elegir familia ni sobrescribir controles
  personalizados sin una acción de preset claramente confirmada.
- Adaptar líneas ya abiertas al cambiar dispositivo/binding; ahora los mensajes
  cambian al volver a resolver su página. Revisar HD/pixel text, ancho, recorte,
  fuentes de símbolos, fallback accesible y mandos múltiples en pruebas GPU.
- La identificación de familia en guests depende del tipo que exponga su shim;
  hoy un tipo genérico conserva el fallback de posición. No afirmar detección
  universal ni paridad visual sin capturas de cada runtime/dispositivo.

Original/GBA permanece apagado por defecto y no cambia el texto original.
No hay porcentaje de cobertura ni promesa de soporte completo por esta entrega.
