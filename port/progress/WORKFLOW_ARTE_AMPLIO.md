# Workflow: completar el arte de la vista amplia (revisión Astra, 21/10/2026)

## Protocolo obligatorio por pieza

Esta revisión convierte los ejemplos aprobados en pasos verificables. No
garantiza calidad por el modelo utilizado: exige abrir evidencia y rechazar
defectos antes de avanzar. Las instrucciones nuevas del usuario se incorporan
a **la misma Preliminar V1**, conservando comentarios y pendientes anteriores.

**Base de trabajo:** continuar desde la versión más avanzada **validada
visualmente por escena**, con commit/captura identificados. Conservar las
mejoras de Astra ya validadas al cambiar de modelo; no volver a un borrador
anterior ni escoger automáticamente el código más reciente. Registrar modelo
real si se conoce, versión, revisor y límites. Las aprobaciones del usuario
mandan sobre una valoración del agente.

Copiar [PLANTILLA_REVISION_FONDO.md](PLANTILLA_REVISION_FONDO.md) por pieza.
Completar los pasos P0–P7 en orden. No iniciar otra implementación hasta cerrar
la actual. Inventario y diagnóstico general sí pueden hacerse en conjunto.

### P0. Abrir las referencias aprobadas

Rutas relativas a `khcom-codex`; son archivos privados, no se añaden a Git.

| Referencia | Imagen real | Qué observar y trasladar al fondo nuevo |
|---|---|---|
| Agrabah 107/108, honores | `../ReKHCoM-prueba-agrabah-107/propuesta-final.png` | Calle `2*y+x` y fachadas `2*y-x` independientes; techo entre ambas; bordillo entero y dos paridades. [Receta](AGRABAH_107_REVISION.md), `port/include/port/port_agrabah_tiles.h`. |
| Agrabah 109, segunda versión, honores | `../ReKHCoM-prueba-agrabah-109/propuesta-v2.png` | Tira intacta en la misma diagonal; después puestos/jarrones completos con espacios variados. [Receta](AGRABAH_109_REVISION.md), `port/include/port/port_agrabah109_tiles.h`. |
| Guard Armor 008, honores | Evidencia documentada en [BATTLE_SCENERY_TECHNIQUE.md](BATTLE_SCENERY_TECHNIQUE.md) | El mismo patio en combate y evento: identificar por mapas, no por modo. Ver [aprobaciones](LOGICA_APROBADOS.md). |

Abrir la imagen, no solo leer esta tabla. Anotar un borde y un objeto único
que se conservan correctamente. Conservar también 004/057/061/074 y demás
aprobados. Si falta la imagen, recuperarla de la entrega/evidencia; no usar un
borrador anterior como si fuera la referencia. Copiar el método, **no las
coordenadas** de Agrabah a otra escena.

### P1. Identificar y guardar el punto de partida

- Vincular cada captura del usuario a su fondo real por personajes,
  arquitectura, `event_index_data.c` y mapas. No asumir que dos imágenes
  consecutivas corresponden a la misma escena.
- Registrar símbolo, hashes regionales, dimensiones almacenadas y rectángulo
  realmente dibujado. Buscar todas las apariciones del mapa y sus fixtures.
- Abrir compuesto y cada capa por separado con transparencia/fondo de
  contraste. `scene_tile_data.py` proporciona mapas, tiles y paletas para
  exportarlas; no deducir donantes solo de la miniatura compuesta.
- Guardar baseline, ejecutable/commit y defectos por zona. Un estado que
  muestre otra escena o un productor bloqueado no cuenta como prueba válida.

**Salida exigida:** identidad, capas, base y lista de defectos en la ficha.

### P2. Medir los planos y sus límites antes de la textura

Marcar pared A, pared B, retorno de esquina, suelo, techo, cielo/agua y vacío
intencionado. Para **cada** borde anotar capa, dos puntos originales separados,
dirección, grosor y fase. Si cambia dirección o intercepto, separarlo en tramos.
Las diagonales isométricas son los ejes rectos del mundo: no sustituirlas por
una franja horizontal o vertical de pantalla.

Declarar si las coordenadas son píxeles o tiles de 8×8. Para píxeles:

| Dirección | Invariante | Muestreo al elegir un donante en `sx` |
|---|---|---|
| Baja hacia la izquierda | `L = 2*y + x` | `sy = y + (x-sx)/2` |
| Baja hacia la derecha | `R = 2*y - x` | `sy = y + (sx-x)/2` |

Ejemplo de cálculo, **no coordenadas de una receta**: por `(0,144)` y
`(192,48)` pasa `L=288`; en `x=-64` la línea debe pasar por `y=176`.
Dos paredes paralelas pueden tener distinto intercepto: conservar el retorno
que las une. Una escena frontal con punto de fuga, como 099, requiere su propia
geometría; no forzarle un 2:1.

**Trampa de media celda:** mover 8 px horizontalmente exige 4 px verticales.
Dividir coordenadas de tiles y truncar esos 4 px corta filas o crea escalones.
Usar traslaciones compatibles (p. ej. 16 px/8 px), o muestreo de píxeles con la
fase de 4 px. Comprobar dos puntos interiores y dos exteriores de cada unión,
incluyendo ambas paridades. El borde, friso y sombra deben seguir juntos.

**No avanzar** si un límite cambia de altura, se vuelve horizontal, pierde
grosor, desaparece o produce una esquina que no existe en el original.

### P3. Registrar donantes completos y remates

La ficha debe declarar por donante: capa/recurso, rectángulo en píxeles y
tiles, plano, período X/Y, ecuación/desfase, elementos excluidos y alternativa
si sale de rango. Inspeccionar todo el rectángulo, incluidas transparencias.

- Clasificar **regiones**, no capas enteras. BG2 puede contener un friso
  repetible y un emblema único: apagar toda BG2 conserva el emblema pero corta
  el friso. Continuar por separado labio, cornisa, sombra y contacto con suelo.
- Si la tira está truncada, avanzar por la misma diagonal a otra intacta.
  En 109, pasar de x=24..31 a x=16..23 y ajustar y. No sustituir media cornisa
  por pared lisa. Si solo hay 8 px útiles y hace falta 4 px de fase, muestrear
  píxeles; no incluir fragmentos de adornos vecinos para ampliar el donante.
- Una textura necesita su módulo de altura completo. Repetir una sola fila
  de ladrillos hacia arriba produce rayas verticales, aunque el color coincida.
- Un fuste que cruza el borde superior continúa en su eje, sin crear otra
  columna. Un relieve cortado exige estudiar/completar la pieza; no ocultarlo
  con una banda. Conservar los objetos únicos en su posición y silueta.
- Revisar cada capa y compuesto. Un hueco transparente puede estar cubierto
  solo dentro del mapa: extender la superficie correcta debajo. No rellenar
  automáticamente todos los vacíos con suelo.

**Salida exigida:** tabla de donantes sin objetos cortados, celdas vacías
inesperadas, esquinas clonadas ni módulos incompletos.

### P4. Implementar y probar primero una muestra

Implementar planos/remates antes del detalle; colores de paleta activa, solo
fuera de 240×160, bajo `PLATFORM_PC`, sin cambiar cámara, colisiones ni OAM.
Identificar por datos, incluidas regiones y EU descomprimida. Compilar EU con
`-j 1`, capturar 3440×1440 a x3 y abrirla antes de correr la matriz larga.

Rechazar aquí: banda horizontal sobre muro diagonal; suelo detrás de pared o
borda; tiras estiradas; espejo que cambia luz/perspectiva; friso interrumpido;
relieves, ventanas o esquinas clonados; rectángulo original pegado sobre otra
textura. No poner decoración encima para esconder una costura.

### P5. Puerta visual: abrir y justificar la evidencia

1. Abrir panoramas 3440×1440 y 3840×1080: planos y silueta deben leerse como un
   mismo lugar. El límite rectangular del mapa no debe delatar el relleno.
2. Revisar uniones izquierda/derecha/arriba/abajo y esquinas a 1:1 de píxeles
   del juego y a x3 sin filtro. Registrar crops con al menos 32 px a cada lado
   de la unión, o más si el módulo/objeto es mayor. Miniaturas no bastan.
3. Seguir cada diagonal desde el original hasta el extremo del panorama,
   incluidas las dos paridades, friso, sombra y suelo por separado.
4. Comprobar objetos únicos completos, piezas ambientales apoyadas, detalles
   que cruzan y=0 y transparencia real de cada capa.
5. Comparar con las referencias de P0; escribir veredicto por zona con archivo
   exacto. «Se ve bien» sin decir qué uniones se miraron no cierra la pieza.

Si queda un defecto estructural o un comentario nuevo sin responder, volver
a P2/P3. **Ningún `passed=true` elimina un defecto visual.**

Contraejemplos confirmados de esta V1:

| Captura real | Defecto rechazado | Qué debía comprobarse |
|---|---|---|
| 121, patio con Hércules y Fil. Portapapeles `codex-clipboard-cd69c223-efa6-4650-aabe-0dc040946f30.png`. | Rayas verticales, friso/columna/relieve cortados. | Dos paredes y retorno, módulos de ladrillo completos y remates de BG2. Esta imagen **no es 122**. |
| 122/123, muro con relieves detrás de Cloud. `codex-clipboard-b5a81de2-34eb-44d3-bc11-d6a0130c4363.png`. | Suelo donde debía seguir el muro; siguiente borrador añadió una banda horizontal enorme. | Planos y quiebres reales; `y<0 = pared` no define la geometría. |

Conservar copia privada de comentarios/capturas, pues `%TEMP%` es transitorio.
Ambos tipos de error pueden pasar las pruebas nativas: no son referencias.

### P6. Puerta técnica: alcance exacto de las comprobaciones

- Misma instantánea y mismo ejecutable antes/después; apagar solo la receta
  actual y mantener las anteriores. Cuatro ventanas x3 y varios estados.
- El harness completo usa 100/300/1000: 12 pares por fixture. `--quick` solo
  toma 300 a 3440×1440: diagnóstico, no cierre completo.
- `native_diff:null` significa que `ImageChops.getbbox()` no encontró cambios
  nativos; **no es dato faltante**. `changed:true` solo dice que algún píxel
  exterior cambió. `READY` y `visual_review:pending` no significan aprobación.
- Probar Original (imagen entera idéntica), EU/US/JP e inicios compartidos.
  Distinguir matriz completa de muestra rápida regional. Entrada/fundido,
  cámara, animación y salida necesitan estados donde realmente ocurran; los
  números 100/300/1000 no garantizan esas fases. Registrar cobertura pendiente.
- Fondos aprobados/anteriores deben permanecer iguales al activar solo la
  receta nueva. Si cambia el renderer común, ampliar controles afectados.
- Después del último cambio renovar evidencia del binario final. No usar un
  informe anterior ni repetir baterías sin un cambio/fallo que lo justifique.

### P7. Cierre y continuidad

La ficha debe contener fuente/planos/donantes, veredicto visual por zona,
matriz/regiones/fases realmente cubiertas, ejecutable/commit y límites.
Actualizar PRELIMINAR_V1 y hacer commit por pieza. Codex commitea; Claude
integra. No pedir permiso entre piezas ni marcar aprobación sin comentario
explícito del usuario. Congelar como base la versión más avanzada validada.

Estados distintos: **borrador**, **geometría revisada**, **propuesta corregida y
comprobada pendiente del usuario**, **aprobada por el usuario**. Una receta
heredada en READY puede reabrirse. Preservar evidencia del motivo de rechazo.

Solo después de ambas puertas copiar evidencia final a `ready` y empaquetar.
El empaquetador actual solo exige `passed` y 12 checks por fixture: **no verifica
calidad visual, identidad del binario ni cobertura regional**. La ficha cubre
esos controles. No añadir ROMs, estados ni imágenes extraídas a Git.

No perder pendientes independientes: actores fuera del viewport nativo
(Cheshire/Aerith), fundido de Aerith y prototipo Original x6 con `.bat`.

Comandos concretos: [ARTE_AMPLIO_COMANDOS.md](ARTE_AMPLIO_COMANDOS.md).

## Reglas de origen y detalles del procedimiento

Pedido del usuario: un solo procedimiento para generar el arte que falta fuera de
los 240x160 originales (salas, escenas, escenarios de combate y sus eventos),
**duplicando tiles del juego y dibujando pixel art solo donde sea necesario**.
Reúne lo aprendido en LOGICA_APROBADOS.md (reglas), BATTLE_SCENERY_TECHNIQUE.md
(escenarios de combate) y PRUEBAS_VISUALES.md (cómo entregar). Ante la duda,
mandan las reglas de LOGICA_APROBADOS.md.

## 1. Encontrar todas las apariciones del fondo

Un mismo fondo sale en más de un sitio: el combate, el evento que lo precede,
la derrota, la revancha. Guard Armor (20/10) tenía el patio aprobado en el
combate, pero la escena 008 (las piezas caen y se arman) usaba el mismo
fondo sin la extensión, porque esta exigía el modo combate.

- Identificar el fondo por sus datos (los mapas o tiles exactos), nunca por el
  modo, el número de combate o una bandera de estado.
- Buscar dónde se carga ese fondo (`grep` del símbolo del mapa en `src/`) y
  probar cada sitio: combate (`KHCOM_BATTLE`), evento (`KHCOM_EVENT=lista:índice`,
  ver `port/tools/test_events.py`), sala (`KHCOM_MODE=...:room...`).

## 2. Leer el arte original

1. Decodificar mapas, tiles y paletas (`KHCOM_ROOMDUMP=1` + `room_ascii.py` en
   salas; en combates, los mapas del cartucho, como `port_guard_armor.c`).
2. Ubicar el rectángulo realmente ilustrado: un mapa de 512x512 puede tener
   zonas vacías; repetir el mapa entero repite también sus huecos.
3. Clasificar cada zona:
   - **Única** (torre, escalera, marco de ventana, relieve, figura, jefe):
     nunca se copia ni se corta.
   - **Banda repetible** (fachada, tejado, arcos, pared lisa, cielo, pavimento):
     se continúa con su propio período y dirección.
   - **Borde** (festón, cornisa, bordillo): cierra el mundo donde el original
     termina en corte recto.
   - **Decoración ambiental repetible** (puestos, grupos de jarrones): piezas
     completas del propio escenario que pueden poblar su continuación cuando
     encajan con el ambiente; distinguirlas de los adornos únicos.

## 3. Generar el arte, en este orden de preferencia

1. **Duplicar tiles del juego**: repetir la banda con su período, voltear un
   tile (bits 0x400 / 0x800 de la entrada) para cerrar un borde, usar la pared
   lisa (el tile de BG3 más repetido de la columna, `MapPlainBaseTile`).
2. **Muestrear píxeles del juego** donde el tile no encaja (suelos en
   perspectiva): fila por fila, con los colores y límites de las celdas
   originales, como el pavimento de Guard Armor.
3. **Dibujar pixel art** solo si no existe tile ni píxel adecuado (autorizado
   por el usuario el 18/10):
   - mismos índices de paleta del fondo (nada de colores nuevos), mismo grosor
     de línea, mismo sombreado y misma rejilla de 8x8;
   - el dibujo continúa el del original (perspectiva, período, luz);
   - va en el repo con una nota de qué tile imita y por qué no bastaba copiar.

### Mejoras aprendidas en Agrabah 107/108/109 (aprobadas con honores, 21/10)

- Cerrar primero la estructura: una tienda no sustituye la reparación de un
  borde cortado. Seguir cada diagonal por separado y conservar todas las filas
  de sus tiles, incluidas las dos paridades de una línea 2:1.
- Comprobar que el donante contiene dibujo en cada columna. Si lo corta el
  límite del mapa, avanzar hacia otra tira intacta por la misma diagonal;
  no reemplazar parte del bordillo por pared lisa.
- Para dar continuidad al ambiente, reutilizar piezas decorativas completas
  del propio juego. Intercalar tipos y grupos, variar las separaciones y dejar
  descansos; mantener anclaje al suelo, perspectiva y escala. No repetir una
  cuadrícula de puestos ni clonar marcos, relieves o figuras únicos. Las
  posiciones concretas se deducen del fondo nuevo, no se copian de Agrabah.
- La decoración adicional es arte de fondo: no añadir objetos de lógica,
  colisiones, personajes o entradas OAM. Respetar la paleta activa de la fuente,
  también cuando la pieza original usa paleta OBJ.
- Comparar la misma instantánea y ejecutable antes/después: el avance del
  texto y la animación pueden diferir entre arranques independientes.
- En cada nueva pieza de la tanda, apagar solo su receta para los controles,
  manteniendo activadas las anteriores. Si una tira limpia mide un tile y la
  diagonal necesita alternar 4 px, usar muestreo de píxeles con ese desfase;
  no recortar el borde ni aceptar fragmentos del adorno vecino como donantes.
- Revisar cada capa exportada por separado además del compuesto: una celda
  transparente puede parecer cubierta por otra capa en el inventario y quedar
  negra en los márgenes ampliados. Rellenar solo los píxeles sin capa visible
  con la superficie que corresponde a su zona (pared arriba, suelo abajo),
  manteniendo únicos los objetos de la capa decorativa.

En todos los casos:
- Guardar **índices de paleta** y resolverlos con la paleta activa en cada
  frame: los fundidos y efectos de color del juego siguen funcionando.
- Dibujar solo fuera de los 240x160; el rectángulo GBA queda idéntico.
- No ampliar colisiones, ni cámara, ni duplicar personajes o enemigos.
- Todo bajo `PLATFORM_PC`, con una variable para apagarlo en las pruebas de
  antes/después (p. ej. `KHCOM_NO_GUARD_PATIO=1`, `KHCOM_TOPCLOSE=0`).

## 4. Probar (el usuario lo pide así)

- **Siempre en x3 entero**: `zoom=6` en `kh_com.ini` (escala 3, píxel entero).
  Una escala grande esconde los fallos de las zonas lejanas.
- Ventanas cada vez más anchas, todas en x3:
  `KHCOM_WINDOW=1920x1080`, `2560x1080`, `3440x1440` (más alto: se ve más
  cielo y suelo) y `3840x1080` (32:9).
- Varios fotogramas de cada fase: entrada, animación propia (caídas, saltos,
  ataques que mueven la cámara), fundidos y salida.
- Mismo ejecutable con la extensión activada y apagada: rectángulo GBA
  idéntico; los demás fondos, idénticos en toda la imagen.
- Revisar a ojo en las capturas anchas: costuras, escalones en diagonales,
  bandas que no casan, adornos cortados o repetidos.
- Compilar y ejecutar las comprobaciones en serie, una tarea de juego a la vez,
  para cuidar RAM/CPU mientras el usuario juega.

## 5. Entregar y cerrar

Pedido actualizado del usuario (21/10): preparar **Preliminar V1** con todas
las escenas que necesitan extensión y todas las peleas de jefes pendientes.
No parar a pedir aprobación de cada pieza. Aplicar el workflow mejorado a toda
la tanda; conservar las versiones ya aprobadas y sus recetas.

**Precisión en la tanda (indicación del usuario, 21/10): terminar una pieza
antes de avanzar a la siguiente.** El inventario puede prepararse en conjunto,
pero cada escena/jefe debe recorrer completo el ciclo de estudiar el original,
elegir donantes, implementar, revisar capturas, corregir, comprobar y cerrar su
entrega. Solo entonces empezar la siguiente. Una compilación general o unas
capturas iniciales no cuentan como piezas terminadas. Mantener una lista
explícita de terminadas y pendientes; no trasladar errores a toda la tanda.

1. Implementaciones reales, listas para integrar: código del renderizador,
   paletas activas, pruebas y documentación. No entregar montajes ni conceptos
   como sustituto de los fondos funcionando en el juego.
2. Copia jugable completa + lanzadores, índice de escenas/jefes y capturas x3
   antes/después en los cuatro anchos. Registrar por pieza qué se comprobó,
   qué cambia y sus límites; los vacíos intencionados no se rellenan por sistema.
3. El usuario comenta el conjunto Preliminar V1. No marcar cada propuesta como
   aprobada ni publicarla por el mero hecho de formar parte de la tanda.
4. Congelar recetas y mejoras en commits revisables. **Codex hace los commits;
   Claude integra**. Registrar las aprobaciones reales en LOGICA_APROBADOS.md.
