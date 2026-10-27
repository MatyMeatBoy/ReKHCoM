# Controles portables y prompts coherentes

Aplicar cuando el producto soporte distintos mandos/teclado o necesite adaptar tutoriales. Separar tres contratos: **acción semántica** (confirmar, cancelar, saltar), **binding físico efectivo** y **texto/glifo mostrado**. Cambiar el dibujo de un botón no debe cambiar silenciosamente la acción ni afirmar un binding inexistente.

Si encaja con el producto, ofrecer **Auto**, teclado, ratón, táctil y dispositivos especializados, con override explícito. Distinguir selección de familia visual y selección/rebinding de controles. Botones primarios de distintos dispositivos pueden representar convenciones seleccionables de confirmación; no asumir que lo sean en todas las regiones, aplicaciones o preferencias. Respetar confirmación/cancelación regional y rebinding del usuario.

Resolver tokens de tutorial/ayuda desde la acción y el mapping efectivo, con su convención seleccionada: un tutorial de confirmar debe mostrar el botón que realmente confirma. Si el texto original embebe nombres de botones, integrar su resolución en el punto apropiado del motor sin reemplazos globales ambiguos. Mantener nombres, glifos y acción coherentes también en menús, errores y accesibilidad.

## Cambio de dispositivo y presentación

En Auto, usar identificación fiable y actividad de entrada pertinente; definir comportamiento ante hotplug, desconexión y entradas mixtas evitando parpadeos de familia por ruido. Conservar el override hasta que el usuario lo cambie. Para dispositivos desconocidos, usar nombres/botones genéricos o teclado según el binding real, sin inventar identidad del mando. No borrar rebinding al cambiar sólo la familia visual.

Localizar nombres y texto, comprobar cobertura de glifos, ancho/escala, contraste y alternativa textual accesible. No depender sólo del color o un símbolo que la fuente no contiene. Reutilizar un atlas pequeño/cacheado y actualizar prompts al cambiar mapping/dispositivo/locale; no cargar imágenes por cuadro. Usar arte propio o con licencia compatible y conservar atribuciones; nombres de familias no conceden derechos sobre iconos o marcas.

## Validación pertinente

Probar acción→binding→prompt en cada familia implementada, override/Auto, convenciones regionales y rebinding. Incluir tutorial real, confirm/cancel, teclado+mandos mezclados, hotplug/desconexión y dispositivo desconocido. Revisar localización/accesibilidad y que invalidar la caché actualice el prompt sin perder input ni introducir cargas por frame. Documentar familias no implementadas o no probadas; no afirmar soporte universal por reconocer un nombre de dispositivo.
