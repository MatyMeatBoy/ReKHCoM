# Atlas: todas las versiones aceptadas, 29/10/2026

Autoridad: `/api/state` local, 82 familias y 78 con approved_version.
Las 78 imagenes conservan su hash y approved_version coincide con build_version.
El JSON hermano guarda una fila por familia y cada eslabon de su cadena.
El validador falla ante una omision, duplicado o sustitucion por una hija pendiente.
Cinco tests comprueban estas condiciones.

## Aprobacion provisional y feedback posterior

Aclaracion humana 29/10: algunas versiones se aprobaron para usarlas mientras
se mejoran. Por tanto approved_version/build_version siguen validas aunque
existan comentarios o current_feedback_status=changes_requested (caso112).
La base aprobada permanece activa; sus comentarios siguen como trabajo pendiente.
No se promueve una hija sin aprobar ni se retira la base aceptada.

## Tres activaciones corregidas

- Trickmaster: c6f2bb42ff5143bf98ee674f64065f71. port_trickmaster.c es identico
  al merge86b053798 de PR151(MERGED). Se cambia solo tmBoard de preview a
  PortArenasApproved; Off=0 conserva el original.
- 081: 8072e8379e404ddd9ba2eac73de50f3c. Sus dos ramas TileSource/PixelSource
  son identicas a a03649691/PR193(MERGED bd784a0b). Faltaba81 en la allowlist.
- 097: 4bdd58353e74419d9d3f5dac83cfc82d. Recetas identicas a PR193;
  conserva la condicion de sombras de PR213 y los fixes compartidos de vida
  de actores de PR214. No se afirma que todo PPU sea identico a PR213.
  Se agrega97 a la allowlist, sin modificar actores ni arte.

Oogie se resolvio aparte en PR510/main67e6704dc; este PR no duplica su cambio.
Son cuatro defectos conocidos, no una declaracion de que toda la auditoria
este cerrada. Hay consumidores pendientes de contraste exacto, listados debajo.
CPU: RED en los3gates; GREEN26escenas/6controles pendientes/12combinacionesTrick.

## Evidencia runtime y limites

Quince procesos EU32, SDLdummy/software, BELOW_NORMAL,1PPUthread,
KHCOM_BACKGROUND_TEST=1:30capturas, todos exit0.
Defaultnuevo=previewanterior6/6; defaultnuevo=previewnuevo6/6;
Offnuevo=defaultanterior6/6, pixel por pixel.
CentroGBA=Off4/6:081 ya cambia su tapa negra interior en la receta aceptada;
se conserva exactamente igual a esa preview. No se afirma centrointacto081.
Binario a8d23407f96fc49a7e65cd41608b4e0190f0590e97171db510e534307636f23e.
Relink mixto declarado con356entradas EU32 existentes READONLY, incluye WIP
journal del integrador; no buildclean/64/GPU/Android. Tiempos7.156-50.234s
con logging/capturas y carga concurrente: no son p95/p99 de stutter.
Evidencia local `pruebas/atlas-v23-approved-gaps/`: build-receipt,
source-equivalence, runs y runtime-comparison. Los PNG de pruebas se conservan;
BMP propios se retiran solo despues de verificar RGBA identico.
Hijas Atlas de verificacion, sin aprobar arte nuevo:
081=b39c0c96f13c45319956f1b5269c5b89;
097=b8283d95194f4206bf078c65de80f512;
Trickmaster=0062700a4a87453792f6463b37ab20ac.

Cobertura fresca de activacion4/78 (Oogie y estos3). Las otras74 no se
certifican en runtime por esta pasada. No se infiere runtime de una fila,
un archivo existente o un PR cerrado. PR130 de Oogie esta CLOSED sin merge.

## Importador, recursos y consumidores

Se audito READONLY el cache actual jiminy23-list-native/native-upgrade:
911recetas corrientes/intentadas/con output, ninguna stale ni ausente.
Todos los PNG pasaron CRC, sin decodificar simultaneamente todas las imagenes.
`gen_hd_sprites_inc.py --check` pasa. El checker real `check_hd_sprites`
con simbolos y ROM EU/US/JP originales READONLY da911filas/0problemas.
Cache:29162805bytes codificados,227999232bytesRGBA potenciales; no es VRAM
medida ni consumo simultaneo. No se cambia resolucion, arte ni filtros.
El catalogo/registry/import completo se cruzo tambien para las28familiasUI,
pero emparejar cada aprobacion exacta con su pantalla/frame actual sigue
pendiente donde se indica. Las50familias nativas usan tiles/paleta, sin
importar las laminas de Atlas como texturas del juego.
Los78PNGs de referencia suman21350482bytes y1352402752bytesRGBA. Son laminas,
a menudo con dos capturas:3440x2880 requiere39628800bytesRGBA. Esos totales
no prueban una causa de tirones ni que el juego cargue esos78PNGs.

Excepcion hdHud investigada: port_config.c fuerza hdHud=0 salvo override de
desarrollo, y Options oculta ITEM_HDHUD. Es una proteccion experimental
historica, no el gate de los glifos de Options: PortUiHdDraw usa hdTitle y
sHdUiFrame tambien se selecciona por hdTitle. Los grupos3/4/5 del registro
usan hdCards. No se activa todoHUD por comentarios antiguos que dicen HUD HD.
Continua la revision exacta de las28UI y sus recursos/core assets.

## Inventario completo

Cada fila conserva el titulo original de Atlas, sin reinterpretar su UTF8.
El JSON registra blobs actuales y referencias historicas; una referencia
textual no prueba equivalencia exacta del arte aceptado.

| Familia | Version exacta | Dimensiones PNG | Bytes PNG | Bytes RGBA | Tipo | Estado de cadena |
| --- | --- | --- | ---: | ---: | --- | --- |
| Darkside 01 · De pie | `b7b6839a2cbc448fbcec91996c9bbb45` | 912x538 | 36179 | 1962624 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Darkside 02 · Agachado | `65700d8493bb4bf99be8721f604872c1` | 912x560 | 40945 | 2042880 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Darkside 03 · Brazo en alto · r6 | `2938cbbb50a345088e08d0365dd0b9f3` | 912x678 | 48020 | 2473344 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Darkside 04 · Garra · r5 | `32d663b982324eccb45104cc4b99b447` | 912x546 | 43353 | 1991808 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Darkside 05 · Inclinado · r5 | `cb75a018337a42ca8e4f492b5d413800` | 912x518 | 42837 | 1889664 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Darkside 06 · Puñetazo | `60f90456598249ecb1e691c51dd2434e` | 912x640 | 42751 | 2334720 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Darkside 07 · Alcanzar · r3 | `24a9a985d92d4c50bd83aa29178ade77` | 1356x534 | 39494 | 2896416 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Malefica 01: cuerpo y montaje nativo | `306aea394d144032932cf1595bf4cbdd` | 2048x1440 | 63556 | 11796480 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Malefica 02: cuerpo y montaje nativo | `57768c3432cb475d9e2f67887c8bbaf6` | 2048x1440 | 65678 | 11796480 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Malefica 03: cuerpo y montaje nativo | `2ca1f374795c4c4f9c78ec82f553fe2d` | 2048x1440 | 63236 | 11796480 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Coliseo 121 · Cloud y Hércules | `9917b6dad433408b870bc5eadf613f35` | 3440x1440 | 254499 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Coliseo 122 · propuesta | `b4d77f38012545f9bc6c8e8517e92a37` | 3440x1440 | 270706 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Coliseo 124 · borrador pausado | `acc5d678014f4ee68ac39f5faaaa16f7` | 3440x1440 | 325411 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 007 | `db18e760371f414080d2638a39d7dfd8` | 3440x1440 | 128465 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 044 | `0ac326ad582f4214b308a5d0c1bc6629` | 3440x1440 | 139539 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 045 | `2f0e9dee739642468bded660684f5355` | 3440x1440 | 125580 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 053 | `553e59d173b14e00bc61c1d513188719` | 3440x1440 | 289152 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 054 | `18f40e19e5814241a966b3348a542977` | 3440x1440 | 132621 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 055 | `ac6109e021c34faa8d47288399992a72` | 3440x1440 | 80372 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 081 | `8072e8379e404ddd9ba2eac73de50f3c` | 3440x2880 | 246225 | 39628800 | native_scene | EU32 runtime verificado |
| Escena 087 | `33dd78c3e05f44648d70624c52ae2cf0` | 3440x2880 | 559154 | 39628800 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 090 | `37a6f9b61f0e427988c96890ce9b4f09` | 3441x1440 | 190656 | 19820160 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 095 | `47e89e1d59374be8aa04ff8e658ee3a6` | 3440x2880 | 428866 | 39628800 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 097 | `4bdd58353e74419d9d3f5dac83cfc82d` | 3440x2880 | 416434 | 39628800 | native_scene | EU32 runtime verificado |
| Escena 098 | `de7e4e0b84f346c3a5cb9fd905010b50` | 3440x1440 | 189064 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 099 - revision turbo | `f092a251c3e44a56b1b4727eb3183855` | 3440x2880 | 194584 | 39628800 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 102 - revision turbo | `3bbd4c0046224adaa320c05dae1b2a4e` | 3440x2880 | 268930 | 39628800 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 104 | `52df9ff43f4e48b4a722240c723bc2f8` | 3440x1440 | 182876 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 110 | `aa5c68ac68e948deaa006b2dfec38897` | 3440x2880 | 374809 | 39628800 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 112 | `994a5761f7f042deaba72d7e716ee14e` | 3440x2880 | 358463 | 39628800 | native_scene | Base aceptada valida; feedback pendiente; runtime pendiente |
| Escena 115 | `edd1e953dd7141ad931a43429c621a42` | 3440x1440 | 217431 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 116 | `4767d7d89689440084a12cf7beeab8b9` | 3440x1440 | 239601 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 117 | `ff9f8d6e16a149708fb8a08c5c80d431` | 3440x1440 | 136690 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 118 | `09dcd37573df49b88edbd71f79e06b82` | 3440x1440 | 244022 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 120 | `1c523468e4fb4d83965c0ef8ba292842` | 3440x1440 | 173077 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Darkside: evidencia runtime de cuerpo y r8 aprobados | `a6ae1438d7524bbe90e3a84952b1dab9` | 3840x2160 | 199813 | 33177600 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Jafar · blinkeo y copia del genio (corrección técnica) | `e182f7052b8348b2a88b4f2e996a0ed7` | 960x1120 | 70623 | 4300800 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Malefica: evidencia runtime de sprites aprobados | `17d1a11e90b1438287c9654c99c49867` | 3840x2160 | 208703 | 33177600 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Marluxia · arena | `c333e5cd5db24eefa4d8f1ff9015843b` | 1920x3276 | 1476208 | 25159680 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Oogie Boogie · arena | `aa913fe773c4446eb5412c3115e222c7` | 1920x2184 | 803912 | 16773120 | native_arena | EU32 runtime verificado |
| Trickmaster - revision turbo | `c6f2bb42ff5143bf98ee674f64065f71` | 1920x3240 | 226515 | 24883200 | native_arena | EU32 runtime verificado |
| Úrsula · arena | `fc418e25bb1e4505aa1ded7f5363a5ce` | 1920x3276 | 1085287 | 25159680 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| 004 y005: ambos encuentros de Leon verificados | `1de7c0eee9be421884c5121e27ead2a9` | 1720x1488 | 880113 | 10237440 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena057: madera superior y matas completas | `853a5ffc4f8c4a7fb4b4e74567f12551` | 3440x2880 | 459321 | 39628800 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 061 | `6bb99e9f57af425f85de491bb77a2716` | 3440x2880 | 450355 | 39628800 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Escena 074 | `d08d6c8758ff4ca9906c89a4c4d95888` | 3840x2160 | 185972 | 33177600 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Agrabah 107 / 108 | `e0779d8a9f5f4eea9eb73e9586505a03` | 3440x1440 | 200192 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Agrabah 109 | `3fa0a50b4582424cbef46d540076c838` | 3440x1440 | 177576 | 19814400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Guard Armor · patio / escena 008 | `7ef20691e77d4bc0b99730ea3443f4af` | 960x270 | 30968 | 1036800 | native_boss | Gate estatico; equivalencia exacta/runtime pendiente |
| Botones táctiles · editor de posición | `b989b29c54114606905d8a677f5210bf` | 1600x2860 | 395262 | 18304000 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Primer arranque · elección de modo con vista previa | `2f30654b7e724653a0b12e104ec1d8b6` | 2820x3114 | 617089 | 35125920 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Menú START · cabecera y márgenes | `40493ca53a21432e9bc348cf527f20f6` | 1920x1080 | 98360 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Ver barajas · cartas, textos y cabecera | `2783c8219d0d4ca4b178db5ab711b9b2` | 1920x2160 | 182681 | 16588800 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Naipes mapa · cartas y cabecera | `2d6f385fd82746dc815ef8a202cf932e` | 1920x1080 | 35524 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Mapamundi · panel, mundos y cabecera | `69729697060d4f1ba33437df81202914` | 1920x1080 | 208226 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Estado · panel y cabecera | `c6f192ad0a2e40b7b96b5fe82e08df08` | 1920x1080 | 100217 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Diario Pepito: lista interior y detalle de carta | `a36793f6231a4c55b3ca636305aea2b7` | 1920x4320 | 331082 | 33177600 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Cargar partida · panel y cabecera | `2a08b34d05f74ff2bf761ca5dd7bf4b7` | 1920x1080 | 83525 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Guardar · panel y cabecera | `08f4e328a8e04249bfcd4bf9da088dca` | 1920x1080 | 83085 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Elige un mundo · panel y cabecera | `42f13762778b463a95d861d37a182348` | 1920x1080 | 41484 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Vórtice · panel y cabecera | `8df3db568b6949a4bacff0c96b7efcdb` | 1920x1080 | 257893 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Tienda moguri · panel y cabecera | `87ef5b5d5a4042fba9f0f2464fc731ae` | 1920x1080 | 78283 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Baraja de Riku · cartas y cabecera | `99c0e57996784d959cff823c7613a051` | 1920x1080 | 220821 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Subida de nivel · panel y cabecera | `5b4933410681483d9d52e665e5f5d68d` | 1920x1080 | 59756 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Síntesis de estancias · cartas y cabecera | `8a3a6caf39214467bbad84b8b18eec62` | 1920x1080 | 261550 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Título · logo HD y barras | `9031374c6e5f43e09d2f50b94725f9eb` | 1920x1080 | 1578395 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| HUD HD · retratos y Contranaipe | `4a39f2318749434ea1602f6d41ef30de` | 798x1956 | 750111 | 6243552 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Agrabah: aparición del Genio (108) | `a5789640eda44d0e9bac19d218cf40a0` | 1920x1080 | 94210 | 8294400 | native_scene | Gate estatico; equivalencia exacta/runtime pendiente |
| Ver barajas: sincronía del margen | `57aed9acb8734e8a9ba35298e646cc7e` | 1536x864 | 25648 | 5308416 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Logros Re:CoM · borde de filigrana e insignias propias | `0d49fe5c171f44c7a4216206d56efc7e` | 1808x576 | 75386 | 4165632 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Bonus élite · cartas y cabecera | `555b3dbca48b4da2a1702cbde1c1b22e` | 1920x1080 | 381192 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Naipes mundo · selección de carta | `3cdc76d68be745b09d5c1bf4eba0c011` | 1920x2160 | 409615 | 16588800 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| HUD de combate · cartas y medidores HD | `d2268d17fc234125bf31446ac44ff213` | 1920x1080 | 170299 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Anadir naipes: cantidades HD detras del selector | `27f14d9fe9bb4428b0dccf98c1e06a8c` | 1536x1614 | 154979 | 9916416 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Sintesis: requisitos dentro de la carta y digitos del mod HD | `823cdf38241144b1b282bcf3f2532120` | 660x746 | 41570 | 1969440 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Tienda moguri · Comprar sobres | `ddedd01659044342acdf92331fa7fddf` | 1920x2160 | 49256 | 16588800 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Tienda moguri · Canjear naipes | `bbfce11b25f34df0a2004146597afb99` | 1920x2160 | 211455 | 16588800 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
| Título · primer plano de la intro HD | `d31ab43ebc4244e793d84b7a140ba11a` | 1920x1080 | 1244694 | 8294400 | menu_hud_or_other | Registry/import911 PASS; consumidor exacto/runtime pendiente |
