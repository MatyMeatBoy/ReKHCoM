# Símbolos semánticos que cambian en una página ya abierta

Carril `controller-prompt-live-page`, owner `codex-control-prompts`. Incremento
encima de #334; **no es refresco universal de todo texto de tutorial**.

El token privado 27 conserva la acción, en vez de congelar el símbolo de una
familia. El token anterior 26 sigue admitido. El layout registra como máximo
128 acciones de slots existentes. Al dibujar, una revisión compara 45 enteros
de contexto: familia visual, normalización Nintendo, inversión L/R, dispositivo
activo, tipo SDL y bindings de teclado/mando. No enumera dispositivos ni carga
imágenes; no cambia por cada botón pulsado si el contexto sigue igual.

Sólo al cambiar esa revisión se sustituyen los handles de tiles cuyo símbolo
es diferente. Conserva X/Y, visibilidad, paletas, argumento de revelado,
speaker, puntero de próxima página y los demás datos de la ventana. El helper
no llama al VM, audio, reapertura ni relayout. Cada tile viejo se libera antes
de cargar el reemplazo; si falla, queda oculto y no muestra el botón anterior
incorrecto. No reintenta asignaciones fallidas cada cuadro. Se vuelve a
resolver al cambiar contexto o al cargar otra página. Free/reset limpia los
tags; no retiene un puntero a un arreglo de glyphs liberado.

La inversión del tutorial de scroll se guarda como flag semántico. Cambiarla
con el tutorial abierto vuelve a resolver L/R sin reescribir texto ni avanzar
la historia. Cambiar sólo familia visual nunca normaliza ni reasigna inputs.

Se reutilizan símbolos de teclas simples de los bindings predeterminados,
con caché fija. Un input no representable muestra `?`, nunca conserva un
Cross/A/otro botón que ya no corresponde. La etiqueta completa sigue
disponible en Controles y los hints con fallback textual. **No se afirma
precisión completa de todos los nombres de teclas dentro de esa celda fija.**

## Prueba pertinente

`test_controller_prompt_layout.py` ahora enlaza las funciones reales de
input/contexto, productor de tiles, layouts Latin/SJIS, refresco y dibujo con
SDL virtual. Verifica:

- 1.000 dibujos sin nuevas cargas de tiles ni revisión cambiante.
- Cross→Circle por rebinding, Xbox, Nintendo, normalización forzada, Original,
  teclado y desconexión mientras la página permanece abierta.
- Cero dibujos si el revelado solicitado es cero; coordenadas, visibilidad,
  paletas y puntero no nulo de próxima página conservados.
- Fallo de reemplazo sin botón obsoleto ni reintentos por cuadro; recuperación
  tras otra revisión; inversión L/R y layout SJIS en vivo.

`test_card_scroll.py` EU/US/JP comprueba que el productor real emite la acción
semántica, con los flags de inversión correspondientes. No necesita ROM,
GPU ni partida. La presentación GPU y el tipo real de mando en guests tienen
validación aparte.

## Subcasos todavía abiertos

- **Original→activar con una página ya compuesta en modo Original:** no hay
  token semántico previo. Requiere metadatos de spans tutoriales aun estando
  apagado, sin alterar su render predeterminado. No reiniciar diálogo/VM.
- **Página inicialmente textual por input no representable:** conserva su
  etiqueta hasta volver a componer la página. El `?` en un símbolo que cambió
  a input no representable es degradación honesta; no cierra el requisito de
  refresco preciso de todo fallback textual.
- Para resolver esos casos, conservar posiciones/ownership/revelado mediante
  offsets de la fuente lógica o spans originales comprobados; nunca usar
  longitud de la cadena transformada para avanzar la historia.
- QA GPU, hotplug real y metadata ABI opcional de hosts 64 siguen pendientes.
