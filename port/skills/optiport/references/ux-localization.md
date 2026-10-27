# Claridad de interfaz y localización

Aplicar cuando la optimización afecte controles, etiquetas, estados, traducciones o previews. Calidad de uso y claridad también son mejoras; no llamar «rendimiento» a todo. Inferir qué representa cada valor y qué hace cada modo desde el código/flujo real antes de cambiar palabras.

Preferir etiquetas que expliquen significado: `26/199` puede ser `Documentos procesados: 26/199` si cuenta documentos completados; no llamarlo importación si no mide archivos importados. Nombrar modos por su efecto y explicar estados de carga, éxito y fallback con precisión. Comunicar una pérdida funcional o fallo accionable sin alarmismo ni esconderlo bajo un éxito genérico. Conservar contexto del dominio: unidades/precisión en ingeniería; perfiles/espacios de color y fidelidad en diseño; estado del trabajo y controles en aplicaciones.

## Traducción verificable

Reutilizar el sistema de localización y los idiomas soportados; no imponer seis idiomas a cualquier proyecto. Si el producto ofrece seis, cubrirlos en los cambios pertinentes. Revisar gramática y convenciones de cada locale, plurales, variables/placeholders, números/unidades y consistencia de términos. No certificar traducción automática sin revisar su significado en contexto.

Mantener encoding esperado (habitualmente UTF-8) y comprobar el repertorio real de fuentes: Unicode válido no garantiza glifo visible. La [lección de fuente JP limitada](verified-lessons.md) muestra por qué una fuente de repertorio limitado puede rechazar kanji aunque acepte kana; aplicar ese mecanismo sólo donde corresponda. Probar textos largos, escalado, ancho disponible, ellipsis con información recuperable y accesibilidad pertinente; no resolver overflow quitando significado o legibilidad. Un gate de glifos no valida traducción, layout ni accesibilidad por sí solo.

## Previews ágiles y honestos

Cuando el flujo necesite previews, mostrar primero un poster/cache válido si existe y cargar el preview después sin bloquear la UI. Identificar si la vista es provisional o de menor calidad; conservar originales y export/calidad final. Invalidar caché al cambiar contenido/parámetros relevantes, cancelar resultados de modos anteriores y acotar memoria, cola y trabajo de carga. Verificar input, cambio de modo y fallback además del tiempo de aparición.

Cantidad de fondos, posters o variantes por modo pertenece al diseño y backlog del proyecto, no a una obligación universal. Elegir sólo recursos útiles al flujo y mantener estado/fallback comprensibles cuando falten.
