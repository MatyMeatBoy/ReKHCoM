# Síntesis: requisito HD dinámico, validado con captura runtime

Tarea `sintesis-requisito-hd`, owner `codex-sintesis`.
Referencia `dbdb82b983fc4a178d46ce70edf44c6b`, envío
`20261025-011932-34d062`. Base del worktree: `d47dbeab6`.

## Corrección preparada para el nuevo turno serial

Feedback `20261025-125559-473c1c`, padre
`c08283ad450f4455937a3f1e1ead3302`: sólo se corrigen las nubes
`a849c7d8-4d91-484e-ba31-1c186915558c` (desborde/distancias/fuente) y
`23d13c7a-d2c6-4911-914a-246099bbafe7` (relleno interior de color).

El generador 405 existente centra y ajusta la tinta de todo el grupo de
dígitos/operador a x=-11..11, dejando margen con el contorno x=±14; no cambia
las cajas OAM ni la lógica dinámica. La banda inferior vuelve a gris neutro
para todas las condiciones; el color queda en el contorno. El centro sigue
transparente. Se observaron la captura GBA `hdm_synthesis_es/out_00470.bmp`,
la propuesta histórica `dbdb82...` y `hdm-check/req_r1.json`: no se ha visto
una captura del propio menú de 1.5HD y no se afirma exactitud con ella.

Hay diez dígitos locales autorizados en
`../khcom-claude-int/build/hdfull/hd/sprites/gCardValueDigitTiles_*.png`.
El staging ahora regenera las piezas con esos dígitos cuando están en el
directorio HD de captura, registra sus hashes y conserva su aspecto/alpha.
No publica los glifos privados del mod como assets del repo: los PNG
versionados son el fallback code-native, también contenido. Con `--hd` al
directorio anterior, la captura usará los dígitos del mod en vez de KHMenu.

Preparados por generación de assets, sin ejecutar tests, compilación ni juego:
- `build/synthesis-fix-assets/`: piezas con los dígitos locales y manifiesto.
- `build/synthesis-fix-assets/asset-preview.png`: sólo plancha de assets,
  no captura runtime ni evidencia nueva de Atlas.
- `synthesis_requirement_evidence.py`: preparada la respuesta a ambas nubes
  del nuevo envío; no se ejecutó ni se subió hija todavía.

La lógica del adaptador, sus 2.000 casos y el pago TOTAL 10→9 no se modifican;
la evidencia ejecutada anteriormente figura más abajo. El fix visual nuevo
queda pendiente del turno serial del coordinador. No se hizo push ni se
actualizó el PR125 antes de una hija real nueva.

Comandos preparados para cuando se asigne turno (NO ejecutados ahora):

```powershell
python port/tools/test_synthesis_requirement_hd.py --build build/pc-atlas --out build/synthesis-fix-final --hd ../khcom-claude-int/build/hdfull/hd/sprites --baseline build/synthesis-final-r2/baseline-off --run
# El binario fixture ya conservado permite reutilizar el pipeline sin recompilar:
python port/tools/test_synthesis_requirement_hd.py --build build/synthesis-variants-r3/blue/hd-on --out build/synthesis-fix-variants/red --hd ../khcom-claude-int/build/hdfull/hd/sprites --fixture red-at-most-7 --run
python port/tools/test_synthesis_requirement_hd.py --build build/synthesis-variants-r3/blue/hd-on --out build/synthesis-fix-variants/green --hd ../khcom-claude-int/build/hdfull/hd/sprites --fixture green-exact-12 --run
python port/tools/test_synthesis_requirement_hd.py --build build/synthesis-variants-r3/blue/hd-on --out build/synthesis-fix-variants/blue --hd ../khcom-claude-int/build/hdfull/hd/sprites --fixture blue-total-10to9 --run
python port/tools/synthesis_requirement_evidence.py
```

El fixture ya fija dummy, PPU1 y mando `0xffff/0xffff`. Tras captura y revisión
visual única, subir hija real con parent `c08283ad450f4455937a3f1e1ead3302`
y las dos respuestas del nuevo envío, ANTES de pushear/actualizar PR125.

## Qué cambia

`MapSelect_2` cubre la puerta normal y `SELMAP_EVKEY_0/2` las llaves de evento,
con el adaptador PC de `src/card/synthesis_requirement_hd.inc`.
Se guarda una copia del requisito
de cada carta al iniciar la tarea. Después de los DrawSprite originales,
se identifican las entradas de dorso y requisito de esa carta. Copias
persistentes de sus ObjTiles cambian únicamente `src`, usado como identidad
por `PortOamSetHdPart`; los tiles VRAM, frames, palettes, índices, flags,
orden y afines son los originales. No se modifica el recurso compartido
ni se cambia cómo se paga una puerta. El recorrido existente dibuja HD y
suprime los píxeles GBA de esas mismas piezas; la PPU no recibe cambios.

El requisito usa `rule/value`; TOTAL usa `drawnTotal`, que ya actualiza
`UpdateEventKeyTotal` con el saldo real. Rangos cubiertos: 0..99, uno/dos
dígitos, al menos/al máximo/igual/total. Fuera de ese rango o si falta alguna
de las dos piezas necesarias, queda la carta GBA completa. NONE sin condición
no agrega un número. Un `kind` específico conserva la ilustración y el
recorrido de cartas existentes: no se inventa un símbolo genérico encima.
Los dorsos distinguen verde/rojo/azul; GOLD conserva la silueta blanca usada
por `gMapCardBackDefs`, y los indicadores de color originales siguen intactos.

No encontré el script original de la propuesta en el `build/hdm-check`
consultado: allí están `req_r1.json` y las imágenes. El nuevo generador
reconstruye la silueta abierta y banda inferior con geometría propia,
supersampling y salida 4x. Los dígitos usan el KHMenu ya incluido en el repo;
no son un recorte del mod de Re:CoM y requieren valoración visual propia.

## Assets y entrega pendiente

`port/assets/synthesis-requirement/`: 405 PNG transparentes y manifiesto.
`port/tools/synthesis_requirement_hd.py`: generador y staging explícito.
`port/include/port/synthesis_requirement_hd_entries.inc`: identidades y cajas
OAM. Se incorpora por el generador de la tabla HD, sin filas falsas de ROM
en el TSV ni cambios al importador de Re:CoM.

Los PNG **no se activan ni empaquetan automáticamente**: son una propuesta
pendiente de captura hija y revisión. Se deben colocar en el directorio de
sprites de la prueba aislada; assets ausentes dejan GBA. La incorporación al
paquete de jugadores se decidirá después de la revisión según la política
de honores. No se ha publicado PR ni se ha enviado una imagen estática como
si fuera evidencia del juego.

## Verificación sin compilación ni juego

```powershell
python port/tools/synthesis_requirement_hd.py --check
python port/tools/gen_hd_sprites_inc.py --check
git diff --check
```

El primer comando comprueba 405 identidades, cajas, dimensiones RGBA y alpha,
y que las cajas incluyan todas las partes de los frames OAM de los YAML.
No demuestra todavía el resultado del renderer ni la sincronía del saldo.

## Reproducir la prueba serial

`port/tools/test_synthesis_requirement_hd.py --build <build-nuevo> --out <dir-nuevo>`
prepara capturas aisladas sin ejecutar; `--hd <sprites-existentes>` mezcla el
resto de los assets HD. Añadir `--run` para ejecutar (usuario autorizó la
prueba serial después de finalizar 004 y Añadir naipes).
`--baseline <build-anterior>` añade una comparación píxel a píxel con HD
apagado. Todo se ejecuta en serie; nunca modifica la partida del usuario.

Fixture: sala 10901 en frame 60, freeze 120, doorpose 200, A 240..242;
capturas 300/380/470/510 a 1920x1080. Logs de ambas piezas nuevas en 470 y
ausencia de ellas en HD off. Captura revisada: sustituye la carta GBA en su
posición, con el foco visible por el centro transparente. Esta puerta es
normal: el primer intento que sólo cubría SELMAP_EVKEY no la reemplazaba;
la captura permitió corregir la ruta real antes de entregar.

También se ejecutaron fixtures runtime para rojo `<=7`, verde `=12` y azul
TOTAL 10→9. Se compilan únicamente al definir `PORT_SYNTHESIS_TEST_FIXTURE`:
`prepare_synthesis_fixture.py` crea un Ninja local que cambia sólo el comando
de `card_worldselect.c`; el build normal no incluye el fixture. Inicializan
recursos con `InitEventKeyCard` real, dibujan por `DrawLayeredCardSprite` y el
adaptador real. TOTAL ejecuta `UpdateEventKeyTotal` en frame490 y sus logs
muestran `total_10` en 470 y `total_09` en 510. Este fixture fuerza el saldo;
no se presenta como una partida natural o un pago de jugador.

El arnés `test_synthesis_requirement_hd_logic.c` usa estructuras reales y el
adaptador incluido directamente. `prepare_synthesis_logic.py` extrae sin
reescribir los cuerpos de `InitEventKeyCard`, `UpdateEventKeyTotal`,
`GetEventKey` y `PayEventKey`, y guarda sus hashes. Asignación y DMA son
dobles controlados; verifica 2.000 selecciones, 20 inicializaciones de
regla/color, HD off/assets ausentes, tipo específico, y la secuencia real
Init TOTAL10 → Pay1 → remaining9 → GetEventKey → Update → selección HD9.
Un pago posterior de 9 completa el requisito real.

```powershell
python port/tools/prepare_synthesis_logic.py
../toolchain/bin/ninja.exe -f build/pc-atlas/synthesis-logic.ninja -j1 build/pc-atlas/test_synthesis_logic.exe
./build/pc-atlas/test_synthesis_logic.exe
```

Build EU completo propio: Ninja `-j1`; generación EU y ROM copiadas mediante
lectura del checkout de referencia. Nunca se compiló sobre un enlace a su
build. Todos los procesos del juego usaron dummy, `KHCOM_PPU_THREADS=1` y
`SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0xffff/0xffff`.

Evidencia local:
- `build/synthesis-final-r2/`: build normal, HD on/off y baseline off;
  los cuatro frames HD off son píxel idénticos al binario de referencia.
- `build/synthesis-variants-r3/{red,green,blue}/`: capturas y logs de variantes;
  son fixtures controlados con inicialización real, no montajes de assets.
- `build/pc-atlas/synthesis_logic_provenance.json`: hashes de funciones reales.
- `synthesis_requirement_evidence.py`: compara de nuevo HD off, crea captura
  principal PNG, plancha de recortes runtime sin retoque y ficha de Atlas en
  `build/synthesis-delivery/`.

La hija real de Atlas usa parent
`dbdb82b983fc4a178d46ce70edf44c6b`, atendiendo nube
`13c9d20d-c78f-4ba3-8431-cd894028c7c0` del envío
`20261025-011932-34d062`.

Subido a Atlas antes de PR, código `943f44a5b99038951169e4c244bcb205edbef711`:
- Captura principal completa: `b77d47de367749538ebe7469598d3926`, hija directa
  de `dbdb82b983fc4a178d46ce70edf44c6b`.
- Evidencia complementaria de variantes: `c08283ad450f4455937a3f1e1ead3302`,
  hija de la captura principal. Ambas incluyen `revision_info.response_to`
  con el envío, padre y nube solicitados; estado humano pendiente.

No se abrió PR ni se activó arte para jugadores. Worktree propio y rama
`codex/sintesis-requisito-hd`, sin cambios a `khcom-claude-int`.

## Límites honestos

Build y pruebas dirigidas ejecutados tras autorización serial. La separación
y oclusión reutilizan código existente; no se declara una batería de todas
las puertas, varias llaves simultáneas o cancelación/reapertura. El pago
real se comprueba en el arnés; la actualización visual se comprueba en un
fixture runtime controlado. La forma y fuente nuevas son una propuesta,
no una reproducción exacta del montaje previo. No se tocó `port_ppu.c`,
`port_hdtext_layout.inc` ni las fuentes/render de texto.

## Entrega por PR

PR #125 abierto y rama pusheada. Propuestas Atlas pendientes de valoración; los assets todavía no se activan ni empaquetan por defecto. La meta2.0 registra esta entrega y las otras cuatro de la tanda.
