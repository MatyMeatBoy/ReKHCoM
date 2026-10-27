# Activar prompts sobre una página compuesta en Original

Claim `controller-prompt-original-live-spans`, owner `codex-control-prompts`.
Rama `feature/controller-original-live-spans`, hija de #338. No es reflow ni
cambio de VM/audio/guion. Este incremento atiende también el P2 de revisión:
una página inicialmente [Return] carecía de token y quedaba obsoleta al conectar
un mando. El productor ahora emite siempre la acción semántica para los tokens
tutoriales reconocidos, incluso cuando su símbolo visible es `?`.

## Original: geometría y ownership

Al pedir el texto de un tutorial conocido en modo Original, se registra sólo
metadata: dirección de la fuente retornada, extensión máxima 512 unidades,
acciones y offsets de las teclas dentro de tokens localizados comprobados.
No se cambia una unidad del texto. Fuera del whitelist o de esa dirección se
rechaza el tag; no se buscan letras A/B/L/R en prosa arbitraria.

Los layouts reales Latin/SJIS guardan esos tags en sus slots existentes. Al
activar la opción, sustituyen únicamente la tecla por la variante compacta de
5x10 del símbolo existente. El ancho mínimo de las letras A/B/L/R/SELECT de la tabla Latin real
es 5 (E), incluyendo continuaciones; la celda SJIS es 10. SELECT/START se sustituyen en la primera letra y se ocultan
sólo sus demás letras, conservando sufijos como Button/botón/ボタン. L+R mantiene
sus anclas y separador; también se reconocen las variantes R+L producidas por
la inversión de scroll. Si un nombre de tecla continúa en la próxima página,
su primera letra visible se convierte en el ancla de esa página.

La variante compacta sólo reduce el bitmap programático 12x10 existente a
5x10; no importa arte ni genera fuentes nuevas. Su caché inmutable añade
18576 bytes estáticos. Metadatos y backups añaden aproximadamente 2.2 KiB en
el guest de punteros32, sin heap por frame. La presentación compacta, sobre
todo símbolos de dos letras, aún requiere QA visual real: pruebas de bounds no
prueban legibilidad.

Cada slot mueve su referencia de tiles original a un backup propio; no la
libera ni inventa un retain. Al desactivar, devuelve exactamente ese handle y
su código, sin cargar tiles. Reset/resto de página restauran antes de que el
layout común libere sus referencias. Free restaura antes del bucle que libera
sprites/paletas. Los followers no cambian visible ni coordenadas: sólo queda
NULL su tile mientras el overlay está activo. Fallo de carga oculta el símbolo
obsoleto y conserva el backup recuperable; no reintenta cada cuadro.

El dibujo Original predeterminado no consulta SDL/contexto ni carga símbolos.
El refresco no cambia X/Y, paletas, visibilidad, n/revelado, nextText, speaker,
ventana o historia. No reinicia ni adelanta la VM.

## Fallback claro y límites

En una página creada con prompts activados, todo token tutorial reconocido es
una celda semántica estable. Teclas comunes representables usan su símbolo;
Return, nombres variables, ejes/posiciones genéricas no representables y binds
vacíos usan `?`. Al cambiar binding/familia/dispositivo se resuelve esa misma
celda: ya no queda una etiqueta textual vieja. El nombre exacto permanece en
Controles/hints. No se promete precisión de todos los nombres de teclado dentro
de 5/12 píxeles y no se hace reflow al cambiar a una etiqueta larga.

Sólo se cubren los patrones ya reconocidos por el parser. Un span no reconocido
y un mensaje mayor que el límite conservan el original; no se afirma cobertura
universal de todo texto ni equivalencia exacta con 1.5. La identificación física
en 64 bits depende de la extensión opcional de #340, separada de este PR.

## Evidencia

`test_controller_prompt_layout.py` enlaza el productor tutorial real, input y
caché reales, layouts/draw/free reales, con SDL virtual y un allocator de tiles
con referencias contadas. EU/US/JP verifican activación/desactivación Original,
restauración exacta, SELECT y continuación, página nueva con backups activos,
fallo de carga y retorno sin allocation, liberación final con cero referencias,
Return inicial→mando sin recomposición y 1000 dibujos estables. El raster
compacto queda dentro de 5x10 y las anclas/continuaciones Latin tienen ancho real mínimo 5.

`test_card_scroll.py --version eu/us/jp` comprueba el productor/input/config
reales; `test_controller_prompts.py` comprueba patrones exactos incluyendo R+L.
Sintaxis de msg.c EU/US/JP, check_sources y diff check pasan. Sin build pesado,
sin instalación protegida. QA GPU/paletas/HD text, mandos físicos y el conjunto
completo de tutoriales siguen pendientes para el cierre 2.1.
