# Atlas: inventario completo de versiones aceptadas, 29/10/2026

Autoridad: respuesta local `/api/state`;82familias,78con `approved_version`.
Se verificaron78/78PNGs contra sus hashes. La selecciÃ³n aceptada coincide con
`build_version` en78/78. Una imagen/historial/PR integrado no certifica activaciÃ³n.

## Tres aprobados adicionales apagados

- Trickmaster: `c6f2bb42ff5143bf98ee674f64065f71`; fuente completa
  `port_trickmaster.c` idÃ©ntica al merge86b053798 de PR151 (MERGED).
  `tmBoard` aÃºn exigÃ­a `PortArenasV2()`; usa el gate aprobado con Off=0.
- 081: `8072e8379e404ddd9ba2eac73de50f3c`; dos ramas de arte idÃ©nticas
  a a03649691/PR193(MERGED bd784a0b); recetaREADY pero faltaba81 en
  `kPreliminarApproved`. Se aÃ±ade sÃ³lo ese nÃºmero.
- 097: `4bdd58353e74419d9d3f5dac83cfc82d`; filas de receta idÃ©nticas
  a PR193. La condiciÃ³n de sombras de PR213(MERGED221780b0) sigue
  presente, con fixes compartidos de vida de actores de PR214. No se afirma
  que todo PPU sea idÃ©ntico a PR213. Se aÃ±ade sÃ³lo97 al gate; arte/actores intactos.

Oogie se entrega por separado en PR510. No se repite ese cambio ni se activan
las hijas pendientes. Los tres fallos se reprodujeron con el C real del gate/lista.
DespuÃ©s,26escenas aceptadas+6controles pendientes+12combinaciones del gate pasan.

## Recursos y alcance

Los78PNGs de referencia suman21350482bytes codificados y1352402752bytesRGBA.
Incluyen lÃ¡minas de varias capturas: el mayor formato3440x2880 requiere39628800
bytesRGBA. No son78texturas cargadas por el juego. Las50familias de escenas/jefes
usan reglas nativas, no un import de estos PNG. Las28familias UI/HUD/otras
requieren comprobar su consumidor e import vigente por separado. Este inventario
no reduce resoluciÃ³n ni aplica filtros; sus tamaÃ±os no prueban la causa de tirones.

La fila112 conserva `approved_version` histÃ³rico pero su estado actual es
`changes_requested`; la base aceptada sigue v�lida para estar activa; los comentarios posteriores permanecen abiertos.
Los otros4grupos no tienen versiÃ³n aceptada: no se promueven.

## Cobertura estática y runtime, sin omisiones

El JSON hermano conserva78filas y el estado de cada eslabón, incluida la
selección exacta de aprobación y el estado actual de feedback por separado.
El validador compara familias/versiones con la autoridad: una fila omitida,
duplicada o sustituida por una hija pendiente hace fallar la herramienta.
Cinco tests cubren esas condiciones, incluido112 changes_requested.

Importador/consumidor: se auditó READONLY el catálogo completo actual de
911recetas, no sólo las cuatro escenas: inventario v2 corriente911/911,
ninguna receta sin intento ni output ausente, PNGs verificadas por CRC.
`gen_hd_sprites_inc.py --check` pasa; el checker real `check_hd_sprites`
con símbolos/ROMs EU/US/JP originales en lectura produce911filas/0problemas.
El cache actual ocupa29162805bytes codificados y227999232RGBA potenciales;
no es una medida de VRAM ni de uso simultáneo. Se conservaron resolución,
recetas y filtros. Ese control global NO prueba que cada frame de cada menú
use la versión Atlas exacta: esa parte permanece explícitamente pendiente.

Hay50familias nativas y28UI/HUD/otras. Se inspeccionaron sus compuertas
actuales y se registraron blobs de fuente/receta;50no importan PNGs de Atlas.
Para las28restantes se cruzó el catálogo/registry/import entero, manteniendo
pendiente el emparejamiento exacto por pantalla/frame y su captura actual.
No se declara equivalencia histórica sólo porque un documento mencione el ID.

Verificación fresca de activación:4/78familias (Oogie y estos3gates); las otras
74no se certifican por esta pasada en runtime. Las pruebas de los3gates son
15procesos/30capturas, todos exit0: defaultnuevo=previewanterior6/6,
defaultnuevo=previewnuevo6/6,Offnuevo=defaultanterior6/6, pixel exacto.
CentroGBA=Off4/6:081ya cambia su tapa negra interior en la receta aprobada,
y se conserva exactamente igual a esa preview; NO se afirma centrointacto081.
Se usóSDLdummy/software,BELOW_NORMAL,1hilo,KHCOM_BACKGROUND_TEST=1.
Binarioa8d23407f96fc49a7e65cd41608b4e0190f0590e97171db510e534307636f23e,
relink mixto declarado con356entradasroot READONLY; no buildclean/64/GPU/Android.
Tiempos7.156–50.234s con capturas/logging y otra carga de revisión: no stutterp95/p99.
Evidencia local `pruebas/atlas-v23-approved-gaps/` conserva receipt de build,
source-equivalence, runs y comparaciones. Atlas hijas de verificación:
081=`b39c0c96f13c45319956f1b5269c5b89`,097=`b8283d95194f4206bf078c65de80f512`,
Trickmaster=`0062700a4a87453792f6463b37ab20ac`; no aprueban arte nuevo.

Excepción de configuración adicional: `port_config.c` fuerza `hdHud=0`
salvo `KHCOM_HD_HUD_DEV`. No significa que los retratos/cabeceras estén
apagados: los grupos3/4/5 usan `hdCards`, no `hdHud`. Se debe rastrear
el alcance exacto/aprobación de Options antes de alterar esa protección.
No se activa todoHUD por esta observación.112conserva su feedback actual y su base aceptada activa, conforme a la aclaraci�n humana del29/10. Los comentarios posteriores no revocan approved_version/build_version ni aprueban hijas.

## Todas las versiones aceptadas

Cada fila conserva el ID exacto, dimensiones y bytes. El informe JSON local
`pruebas/atlas-v23-approved-inventory-after-source.json` aÃ±ade hashesPNG/blob,
fuentes actuales y referencias histÃ³ricas. Esas referencias por sÃ­ solas no prueban
equivalencia de arte ni runtime; las verificaciones pendientes siguen pendientes.

| Familia | VersiÃ³n exacta | PNG dimensiones | Bytes PNG | Bytes RGBA | Tipo | Estado de cadena |
| --- | --- | --- | ---: | ---: | --- | --- |
| Darkside 01 Â· De pie | `b7b6839a2cbc448fbcec91996c9bbb45` | 912x538 | 36179 | 1962624 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Darkside 02 Â· Agachado | `65700d8493bb4bf99be8721f604872c1` | 912x560 | 40945 | 2042880 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Darkside 03 Â· Brazo en alto Â· r6 | `2938cbbb50a345088e08d0365dd0b9f3` | 912x678 | 48020 | 2473344 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Darkside 04 Â· Garra Â· r5 | `32d663b982324eccb45104cc4b99b447` | 912x546 | 43353 | 1991808 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Darkside 05 Â· Inclinado Â· r5 | `cb75a018337a42ca8e4f492b5d413800` | 912x518 | 42837 | 1889664 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Darkside 06 Â· PuÃ±etazo | `60f90456598249ecb1e691c51dd2434e` | 912x640 | 42751 | 2334720 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Darkside 07 Â· Alcanzar Â· r3 | `24a9a985d92d4c50bd83aa29178ade77` | 1356x534 | 39494 | 2896416 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Malefica 01: cuerpo y montaje nativo | `306aea394d144032932cf1595bf4cbdd` | 2048x1440 | 63556 | 11796480 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Malefica 02: cuerpo y montaje nativo | `57768c3432cb475d9e2f67887c8bbaf6` | 2048x1440 | 65678 | 11796480 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Malefica 03: cuerpo y montaje nativo | `2ca1f374795c4c4f9c78ec82f553fe2d` | 2048x1440 | 63236 | 11796480 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Coliseo 121 Â· Cloud y HÃ©rcules | `9917b6dad433408b870bc5eadf613f35` | 3440x1440 | 254499 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Coliseo 122 Â· propuesta | `b4d77f38012545f9bc6c8e8517e92a37` | 3440x1440 | 270706 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Coliseo 124 Â· borrador pausado | `acc5d678014f4ee68ac39f5faaaa16f7` | 3440x1440 | 325411 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 007 | `db18e760371f414080d2638a39d7dfd8` | 3440x1440 | 128465 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 044 | `0ac326ad582f4214b308a5d0c1bc6629` | 3440x1440 | 139539 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 045 | `2f0e9dee739642468bded660684f5355` | 3440x1440 | 125580 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 053 | `553e59d173b14e00bc61c1d513188719` | 3440x1440 | 289152 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 054 | `18f40e19e5814241a966b3348a542977` | 3440x1440 | 132621 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 055 | `ac6109e021c34faa8d47288399992a72` | 3440x1440 | 80372 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 081 | `8072e8379e404ddd9ba2eac73de50f3c` | 3440x2880 | 246225 | 39628800 | native_scene | EU32 runtime verificado |
| Escena 087 | `33dd78c3e05f44648d70624c52ae2cf0` | 3440x2880 | 559154 | 39628800 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 090 | `37a6f9b61f0e427988c96890ce9b4f09` | 3441x1440 | 190656 | 19820160 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 095 | `47e89e1d59374be8aa04ff8e658ee3a6` | 3440x2880 | 428866 | 39628800 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 097 | `4bdd58353e74419d9d3f5dac83cfc82d` | 3440x2880 | 416434 | 39628800 | native_scene | EU32 runtime verificado |
| Escena 098 | `de7e4e0b84f346c3a5cb9fd905010b50` | 3440x1440 | 189064 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 099 - revision turbo | `f092a251c3e44a56b1b4727eb3183855` | 3440x2880 | 194584 | 39628800 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 102 - revision turbo | `3bbd4c0046224adaa320c05dae1b2a4e` | 3440x2880 | 268930 | 39628800 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 104 | `52df9ff43f4e48b4a722240c723bc2f8` | 3440x1440 | 182876 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 110 | `aa5c68ac68e948deaa006b2dfec38897` | 3440x2880 | 374809 | 39628800 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 112 | `994a5761f7f042deaba72d7e716ee14e` | 3440x2880 | 358463 | 39628800 | native_scene | Base aceptada v�lida; feedback pendiente; runtime pendiente |
| Escena 115 | `edd1e953dd7141ad931a43429c621a42` | 3440x1440 | 217431 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 116 | `4767d7d89689440084a12cf7beeab8b9` | 3440x1440 | 239601 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 117 | `ff9f8d6e16a149708fb8a08c5c80d431` | 3440x1440 | 136690 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 118 | `09dcd37573df49b88edbd71f79e06b82` | 3440x1440 | 244022 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 120 | `1c523468e4fb4d83965c0ef8ba292842` | 3440x1440 | 173077 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Darkside: evidencia runtime de cuerpo y r8 aprobados | `a6ae1438d7524bbe90e3a84952b1dab9` | 3840x2160 | 199813 | 33177600 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Jafar Â· blinkeo y copia del genio (correcciÃ³n tÃ©cnica) | `e182f7052b8348b2a88b4f2e996a0ed7` | 960x1120 | 70623 | 4300800 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Malefica: evidencia runtime de sprites aprobados | `17d1a11e90b1438287c9654c99c49867` | 3840x2160 | 208703 | 33177600 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Marluxia Â· arena | `c333e5cd5db24eefa4d8f1ff9015843b` | 1920x3276 | 1476208 | 25159680 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Oogie Boogie Â· arena | `aa913fe773c4446eb5412c3115e222c7` | 1920x2184 | 803912 | 16773120 | native_arena | EU32 runtime verificado |
| Trickmaster - revision turbo | `c6f2bb42ff5143bf98ee674f64065f71` | 1920x3240 | 226515 | 24883200 | native_arena | EU32 runtime verificado |
| Ãšrsula Â· arena | `fc418e25bb1e4505aa1ded7f5363a5ce` | 1920x3276 | 1085287 | 25159680 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| 004 y005: ambos encuentros de Leon verificados | `1de7c0eee9be421884c5121e27ead2a9` | 1720x1488 | 880113 | 10237440 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena057: madera superior y matas completas | `853a5ffc4f8c4a7fb4b4e74567f12551` | 3440x2880 | 459321 | 39628800 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 061 | `6bb99e9f57af425f85de491bb77a2716` | 3440x2880 | 450355 | 39628800 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Escena 074 | `d08d6c8758ff4ca9906c89a4c4d95888` | 3840x2160 | 185972 | 33177600 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Agrabah 107 / 108 | `e0779d8a9f5f4eea9eb73e9586505a03` | 3440x1440 | 200192 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Agrabah 109 | `3fa0a50b4582424cbef46d540076c838` | 3440x1440 | 177576 | 19814400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Guard Armor Â· patio / escena 008 | `7ef20691e77d4bc0b99730ea3443f4af` | 960x270 | 30968 | 1036800 | native_boss | Gate estático registrado; equivalencia/runtime pendiente |
| Botones tÃ¡ctiles Â· editor de posiciÃ³n | `b989b29c54114606905d8a677f5210bf` | 1600x2860 | 395262 | 18304000 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Primer arranque Â· elecciÃ³n de modo con vista previa | `2f30654b7e724653a0b12e104ec1d8b6` | 2820x3114 | 617089 | 35125920 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| MenÃº START Â· cabecera y mÃ¡rgenes | `40493ca53a21432e9bc348cf527f20f6` | 1920x1080 | 98360 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Ver barajas Â· cartas, textos y cabecera | `2783c8219d0d4ca4b178db5ab711b9b2` | 1920x2160 | 182681 | 16588800 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Naipes mapa Â· cartas y cabecera | `2d6f385fd82746dc815ef8a202cf932e` | 1920x1080 | 35524 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Mapamundi Â· panel, mundos y cabecera | `69729697060d4f1ba33437df81202914` | 1920x1080 | 208226 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Estado Â· panel y cabecera | `c6f192ad0a2e40b7b96b5fe82e08df08` | 1920x1080 | 100217 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Diario Pepito: lista interior y detalle de carta | `a36793f6231a4c55b3ca636305aea2b7` | 1920x4320 | 331082 | 33177600 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Cargar partida Â· panel y cabecera | `2a08b34d05f74ff2bf761ca5dd7bf4b7` | 1920x1080 | 83525 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Guardar Â· panel y cabecera | `08f4e328a8e04249bfcd4bf9da088dca` | 1920x1080 | 83085 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Elige un mundo Â· panel y cabecera | `42f13762778b463a95d861d37a182348` | 1920x1080 | 41484 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| VÃ³rtice Â· panel y cabecera | `8df3db568b6949a4bacff0c96b7efcdb` | 1920x1080 | 257893 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Tienda moguri Â· panel y cabecera | `87ef5b5d5a4042fba9f0f2464fc731ae` | 1920x1080 | 78283 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Baraja de Riku Â· cartas y cabecera | `99c0e57996784d959cff823c7613a051` | 1920x1080 | 220821 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Subida de nivel Â· panel y cabecera | `5b4933410681483d9d52e665e5f5d68d` | 1920x1080 | 59756 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| SÃ­ntesis de estancias Â· cartas y cabecera | `8a3a6caf39214467bbad84b8b18eec62` | 1920x1080 | 261550 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| TÃ­tulo Â· logo HD y barras | `9031374c6e5f43e09d2f50b94725f9eb` | 1920x1080 | 1578395 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| HUD HD Â· retratos y Contranaipe | `4a39f2318749434ea1602f6d41ef30de` | 798x1956 | 750111 | 6243552 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Agrabah: apariciÃ³n del Genio (108) | `a5789640eda44d0e9bac19d218cf40a0` | 1920x1080 | 94210 | 8294400 | native_scene | Gate estático registrado; equivalencia/runtime pendiente |
| Ver barajas: sincronÃ­a del margen | `57aed9acb8734e8a9ba35298e646cc7e` | 1536x864 | 25648 | 5308416 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Logros Re:CoM Â· borde de filigrana e insignias propias | `0d49fe5c171f44c7a4216206d56efc7e` | 1808x576 | 75386 | 4165632 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Bonus Ã©lite Â· cartas y cabecera | `555b3dbca48b4da2a1702cbde1c1b22e` | 1920x1080 | 381192 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Naipes mundo Â· selecciÃ³n de carta | `3cdc76d68be745b09d5c1bf4eba0c011` | 1920x2160 | 409615 | 16588800 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| HUD de combate Â· cartas y medidores HD | `d2268d17fc234125bf31446ac44ff213` | 1920x1080 | 170299 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Anadir naipes: cantidades HD detras del selector | `27f14d9fe9bb4428b0dccf98c1e06a8c` | 1536x1614 | 154979 | 9916416 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Sintesis: requisitos dentro de la carta y digitos del mod HD | `823cdf38241144b1b282bcf3f2532120` | 660x746 | 41570 | 1969440 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Tienda moguri Â· Comprar sobres | `ddedd01659044342acdf92331fa7fddf` | 1920x2160 | 49256 | 16588800 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| Tienda moguri Â· Canjear naipes | `bbfce11b25f34df0a2004146597afb99` | 1920x2160 | 211455 | 16588800 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
| TÃ­tulo Â· primer plano de la intro HD | `d31ab43ebc4244e793d84b7a140ba11a` | 1920x1080 | 1244694 | 8294400 | menu_hud_or_other | Registry/import911 PASS; frame/runtime pendiente |
