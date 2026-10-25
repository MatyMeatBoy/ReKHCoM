# Meta para la v2.0 — 25/10/2026

Encargo vigente del usuario: cerrar las correcciones comentadas de Atlas y los bugs actuales antes de preparar la estable. Este documento fija el alcance y el orden; las reservas y los comentarios vivos siguen en la cola remota y Atlas. No implica publicar una release.

## Prioridad inmediata, en este orden

- [ ] **Trickmaster · arena:** responder las dos nubes del envío `20261025-060416-21533f`: continuidad del patrón del muro de la chimenea hacia arriba. Una pasada concreta, captura y comprobación; evitar iteraciones interminables.
- [ ] **Escena 099:** aplicar la misma lógica de la habitación de Trickmaster: prolongar el muro de la casa hacia arriba, sin convertirlo en un techo ajedrezado; conservar chimenea única y suelo.
- [ ] **Escena 004:** completar la muralla derecha; comprobar también la otra escena de León que reutiliza el fondo y enviarla para revisión, como pide su nube.
- [ ] **Síntesis de estancias · cartas y cabecera:** reemplazar la carta/requisito GBA por la nueva pieza HD e integrarla con la lógica real. Número, suma, color y condiciones combinadas deben actualizarse según la sala; no basta una imagen fija. Padre `dbdb82b983fc4a178d46ce70edf44c6b`.
- [ ] **Añadir naipes: cantidad HD y selector:** composición y orden de capas como el original: el selector pasa delante y oculta el texto y las cantidades HD que quedan detrás. Comprobar movimiento y cambios de cantidad para que la transición sea fluida.
- [ ] Después, terminar los cambios de las **14 piezas ya comentadas** antes de tomar piezas nuevas: jefes/arenas primero y, dentro del grupo, del comentario más antiguo al más nuevo. El número histórico no sustituye el estado vivo de Atlas.

Una entrega exige cambio real, revisión visual, captura hija vinculada a las nubes y PR con pruebas. Conservar valoraciones humanas. «Aprobado con mejoras» cierra la versión aprobada; las mejoras futuras no bloquean por defecto, salvo petición expresa del usuario como las anteriores.

## Bugs y comprobaciones técnicas vigentes

Estado actualizado contra `origin/main` (`d47dbeab6`, lista limpia del 25/10) y GitHub. No reabrir como pendientes de código las soluciones ya integradas por otro carril.

- [ ] **#83 Android: confirmar corrección de recogida de carta de Jafar.** Main ya incluye el arreglo BIOS_PTR; siguen la comparación PC vieja/nueva, la validación Android y la confirmación del jugador. El issue permanece abierto. Consultar su responsable; no cerrarlo por pruebas PC.
- [ ] **Android:** repetir batalla123 US (timeout), validar gama baja, tablets, táctiles también en web y elección inicial de modo, cuando los recursos permitan esas pruebas.
- [ ] **Baterías:** consultar `run_batteries.py --status`; el traspaso limpio informa combates EU y Android relanzados, después pasada `--hd`. El resultado histórico86/97 no es un resultado vigente confirmado; investigar cualquier fallo que siga reproduciéndose.
- [ ] **Cargas/transiciones:** consultar la reserva `transiciones-cargas` y confirmar qué falta de primeras cargas visibles y tirones; no duplicar el carril ni reabrirlo solo por una nota antigua.
- [ ] **Fuentes:** verificar si siguen los fallos observados por glifos japoneses ausentes en `port_ui.c`; compilación correcta no equivale a `check_sources.py` correcto.
- [x] **Mazo e invocaciones:** el traspaso nuevo informa el arreglo del parpadeo integrado. Volver a trabajar solo con fallo reproducible.
- [x] **Vídeos HD Linux:** integrados según la lista limpia de main; verificar paquetes finales, sin mantener la antigua rama como pendiente de integración.
- [x] **Pre15 local:** paquete y copia jugable ya preparados por el otro carril. Preparar una siguiente pre solamente cuando integre las entregas nuevas de esta meta.

Pruebas seriales, ocultas y silenciosas, Ninja `-j 1` y consumo de RAM bajo. Todo timeout vigente se investiga; no dar por repetida una comprobación por copiar un traspaso.

Los problemas que esperan datos (cara HD de Sora recortada al recibir daño, cuadro negro en Agrabah con HUD HD, Proton y regresiones de audio #42) siguen como seguimiento condicionado a un reporte reproducible. Switch y online completo continúan después de la v2.0.

## Entregas de esta tanda

| Trabajo | Atlas hija | PR | Estado |
|---|---|---|---|
| Meta2.0 | Documento en este PR | #120 | Documentado |
| Arena Trickmaster | `7c6c539b5c264f24a9b655a8432e2cc7` | #121 | Propuesta entregada; GBA4/4 idéntico |
| Escena099 | `ac568c9ae6c642beb8990ac107cd7a0c` | #122 | Propuesta entregada; GBA2/2 idéntico |
| Escenas004/005 | `8827dd92eb9c4392876f611cbf4cfcb2` | #123 | Propuesta entregada; GBA3/3 idéntico |
| Añadir naipes | `27f14d9fe9bb4428b0dccf98c1e06a8c` | #124 | Propuesta entregada;61frames de oclusión correctos |
| Síntesis | `b77d47de367749538ebe7469598d3926` y `c08283ad450f4455937a3f1e1ead3302` | #125 | Propuesta runtime entregada;2.000 combinaciones,20 inicializaciones, pago10→9 y HDoff idéntico4/4 |

Subir una propuesta no resuelve automáticamente la valoración ni equivale a integración.

## Ya integrado; no volver a contar como pendiente

- [x] PR #111: escena 112, puerta alta naranja y jamba exterior prolongada. Merge `ef79cdb0123dc07e0d88b4abf73f4950badb8eca`.
- [x] PR #114: primera extensión del muro superior de Trickmaster. Merge `7c3709cd81a68e421ec20144ccf91acdf74b829a`. Las dos nubes posteriores son el primer trabajo de esta meta.
- [x] Integraciones previas de arena Trickmaster #105, poses Darkside #106, Oogie #107 y escena 120 #108. Los comentarios posteriores se atienden por separado.

## Cierre de la v2.0

- [ ] Integrar por PR los cambios verificados; no push directo a main. Un solo integrador con reserva remota.
- [ ] Preparar una pre/ZIP local limpio desde el commit final, sin ROMs, y copia jugable; conservar releases y partidas existentes.
- [ ] Ejecutar las comprobaciones de publicación de `PENDIENTES_VIGENTES.md`, fuentes, audio/vídeos HD, launcher, actualización y humo de los paquetes de cada plataforma incluida. Switch queda fuera.
- [ ] Publicar la estable solamente con autorización del usuario. Cerrar issues por solución verificada, no por haberlos anotado en esta meta.

## Fotografía de Atlas al retomar

Estos son envíos con respuesta pendiente en la API de Atlas, no una lista automática de bloqueantes de la estable. Una aprobación no desaparece por figurar aquí. Consultar nubes y fechas exactas antes de tomar cada pieza.

| Pieza | Última imagen | Valoración | Comentario más antiguo sin responder |
|---|---|---|---|
| Agrabah 107 / 108 | `e0779d8a9f5f4eea9eb73e9586505a03` | approved | 2026-10-24 |
| Añadir naipes: cantidad HD y selector | `7116d737f2284b269ee859ebaf65596f` | pending | 2026-10-24 |
| Darkside 06 · Puñetazo | `4d6a039e51ad4b8f8f52cb1b1d070ba0` | approved | 2026-10-24 |
| Darkside · arena y contexto | `9bdf48c45429486fbeb750cfd32cde3f` | approved | 2026-10-25 |
| Diario · panel y cabecera | `02fda8dbbd2b44d7b2cbb26fb41223af` | approved | 2026-10-24 |
| Escena 004 · integrada | `d39a773a8c174176b3870d672367d05d` | pending | 2026-10-24 |
| Escena 057 · integrada | `e49d2f465f9346d28ba013d9a81de938` | approved | 2026-10-24 |
| Escena 061 · integrada | `f31927d3f7014eb3a51e52fcf7323003` | approved | 2026-10-24 |
| Escena 074 · integrada | `303416c343704456aea87be42f0508bf` | approved | 2026-10-24 |
| Escena 081 | `d03972341e324de2838eb705e2deb7b0` | pending | 2026-10-24 |
| Escena 087 | `52879b0b9b9a4d578adcc36ce568601f` | approved | 2026-10-24 |
| Escena 090 | `37a6f9b61f0e427988c96890ce9b4f09` | approved | 2026-10-24 |
| Escena 095 | `7d7e94537fb94d2d892df07eda21705f` | approved | 2026-10-24 |
| Escena 097 | `ec8c60b820a847aeabe9778e8dce033d` | pending | 2026-10-25 |
| Escena 099 | `ae36f45ef10e45cc809620075f0c8626` | pending | 2026-10-24 |
| Escena 102 | `dd5a82c201a84ca29180026bc7cf064a` | approved | 2026-10-25 |
| Escena 110 | `179b6da22e5c4bd39181d5ab14d7b9d3` | approved | 2026-10-24 |
| Escena 112 | `9c486ae93d5244149ada3b6936d312c6` | approved | 2026-10-25 |
| Escena 115 | `edd1e953dd7141ad931a43429c621a42` | approved | 2026-10-25 |
| Escena 116 | `4767d7d89689440084a12cf7beeab8b9` | approved | 2026-10-25 |
| Escena 117 | `ff9f8d6e16a149708fb8a08c5c80d431` | approved | 2026-10-25 |
| Escena 118 | `09dcd37573df49b88edbd71f79e06b82` | approved | 2026-10-25 |
| Escena 120 | `1c523468e4fb4d83965c0ef8ba292842` | approved | 2026-10-25 |
| Maléfica 01 · ala recortada | `4d253e4072d34f03b7569a987702c511` | pending | 2026-10-24 |
| Maléfica 02 · ala recortada | `1541937df30b4f81a4734a58a5829c0e` | pending | 2026-10-24 |
| Maléfica 03 · ala recortada | `d2ec0b82f211456fb830cf2c0ce4e797` | pending | 2026-10-24 |
| Maléfica · arena y contexto | `34fb409fcccc44eeb349f5a177b17cfd` | approved | 2026-10-25 |
| Menú START · cabecera y márgenes | `40493ca53a21432e9bc348cf527f20f6` | approved | 2026-10-24 |
| Naipes mundo · selección de carta | `0e705199e35740d7a0d5f85dba579621` | approved | 2026-10-25 |
| Oogie Boogie · arena | `aa913fe773c4446eb5412c3115e222c7` | approved | 2026-10-25 |
| Síntesis de estancias · cartas y cabecera | `dbdb82b983fc4a178d46ce70edf44c6b` | pending | 2026-10-25 |
| Trickmaster · arena | `5a6c86fcfd4b49a19009d73f9afde97b` | approved | 2026-10-25 |
| Título · logo HD y barras | `c4e96953065648208e79a8484b6b0197` | approved | 2026-10-24 |
| Ver barajas · cartas, textos y cabecera | `f809c0cd1b7242918a735c80fa06dd2b` | approved | 2026-10-24 |
