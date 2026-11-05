# Biblioteca130: extensión y staging pendientes de valoración

Petición «no la extendiste pues»: original Atlas
`c1ab45945afe471fb5f3f6f4b089e1e6`; propuesta offline
`e0a2151657fa4f15a4b9b7dc798fd28a`; primera hija runtime
`e58f861b8d1e44248852242d2f437d36`. La nueva entrega comparativa debe ser
hija de esta última, pending, sin reescribir imágenes ni heredar aprobaciones.

La candidata continúa estanterías con traslaciones (64, ±32) y la alfombra
con su módulo completo (96,48)/(96,-48), conservando escritorio y píxeles
originales. Sólo coordenadas, sin bitmaps ROM en el repositorio. Los hashes
BG3/BG2 son `0x0fdacfc6` / `0xfee677ce`, mapas512². La extensión permanece
optativa (`KHCOM_PRELIMINAR_V1=1`), fuera de la lista de aprobadas.

El arreglo de staging también se aplica sin esa extensión: sólo al evento
`EVENT_130_HOLLOWBASTION_E1` (EU id128; menú0:127). La paleta humana real
es `gMarefPalette`, desde `gEvtObjResources[30]`. Maléfica usa rol estable5;
los arrays tienen6 posiciones, reservando3Hook y4Jiminy. Se conserva Hook
aprobado y los helpers116/117 de maincbdb90b20.

Las sombras usan gráficos compartidos. `PortOamSetSource` captura el origen
exacto de SpriteEntry, ya calculado al emitir cada pieza. El cuerpo y la sombra
de Maléfica comparten ese origen en track4 (z=groundZ=0). No se agrupan por
cercanía ni por slot OAM reciclado. Otras sombras/OBJ no desbloquean ni heredan
el rol. Las piezas que cruzan el encuadre nativo se conservan enteras; Original
y píxeles de juego dentro de GBA permanecen iguales.

Verificación EU32:

- Resolver C igual a la propuesta offline1024×768,139.324píxeles originales
  conservados y todos los donantes residentes en ROM (no tile1023 ausente).
- Fixture C del módulo de alfombra/estanterías, prueba del tagger/filtro real
  y de ownership exacto (un origen cercano pero distinto no es dueño).
- Fuente combinada con main:4tests130/118/registroPASS, sin skips; PPU
  compilado/enlazado con356objetos release intactos.
- Premerge: PPUcompleto spill/state y206oráculos independientesPASS.
  No se afirma que se repitieran206después del merge: el guard1400MiB lo
  impidió. Cuatro funciones130/tagger conservaron SHAexacto al combinar main;
  la interacción de roles118/130 se probó directamente en C combinado.
- 18 pares Extendida y 18 pares Original, desde render300hasta2500, con reloj y
  posición de actor iguales entre variantes.36recortes centrales idénticos.
  Las18imágenes COMPLETAS DEL JUEGO Original (viewport240,60..1680,1020 en
  salida1920×1080) son idénticas. Sólo el icono animado del guardado diagnóstico,
  fuera del juego en el letterbox30..45/27..42, varió. No se afirma igualdad de
  toda la presentación física con ese diagnóstico activado.
- Entrega limpia sin KHST/notificaciones: cinco pares antes/solución a
  frames300,600,2430,2435,2500, misma escala2, núcleoGBAidéntico. La solución
  limpia conserva todos los píxeles del probe salvo su notificación omitida.
  El mosaico público compara los frames600y2500 completos, sin referencias
  ROM puras ni avisos de diagnóstico.

El reloj de secuencia es distinto al del render:2375=>secuencia1100 todavía
(500,500);2376=>1101 ya(359,240). AdvanceEventCharaKeyframe aplica la pose del
siguiente keyframe al avanzar. El cruce de la caja OAM nativa se observa en
2430y la entrada visible continúa2435..2500. No se retrasa una entrada real ni
se cambian tiempos/posiciones del evento.

Evidencia privada `pruebas/atlas-final-20261105/`:
`LIBRARY130-STAGING-NATIVE-CHECKS.json`, `LIBRARY130-CLEAN-SOLUTION-CHECKS.json`,
`LIBRARY130-BLOCKS-COMBINED.json`, `SCENE130staging-PREVIEW-BUILD.json`,
`SCENE130combined-PREVIEW-BUILD.json`, `SCENE130-STAGING-PPU-REGRESSION.json`
y `library130-final-before-solution.png`.

La entrega2935 sigue pendiente de valoración humana. Conservaba la muesca
verde original; la revisión nueva del parent e58 pide completar ese remate.
No certifica todos los fotogramas, otras plataformas ni cambios en
la instalación/release. ROM/SAV originales y revisiones anteriores intactos.


## Remate marcado 05/11

Parent `e58f861b8d1e44248852242d2f437d36`, revisión3 aprobada con general
«falta solo el cachito marcado», doodle `bf2652d8-8382-4cac-ba8f-55a7808e6656`.
La hija2935 no lo cubría ni hereda esa aprobación. Recuperación CAS de
`atlas-final-library130` a `codex-atlas130-final`, `fadc887d2dced2505a2ce58f132e73c93c3f70c5`.

La máscara contiene476píxeles originales de piso verde plano con BG2
transparente. El donante(64,-32) continúa toda la banda del zócalo; se elige
antes de la frontera de alfombra para conservar su ornamento completo. Los476
píxeles cambian en el estudio offline y todo el resto permanece idéntico.
Todos los donantes están cargados desde ROM: banco8352bytes, máximos tile203
BG3 ytile75 BG2; no usa ningún slot virtual o desconocido. Esta excepción
expresamente pedida conserva los otros138848de139324píxeles autorados.
El escritorio, otros muebles y el staging de Maléfica no cambian.

La primera variante92 fue descartada como solución: completaba sólo el
listón de madera y dejaba la banda ornamental cortada. Nunca se publicó en
Atlas. El fixture final reprodujo RED en47,272 y quedó GREEN para1920
coordenadas cercanas, BG2 opaco y otra textura. Preview final EU32 compila y
enlaza sin alterar objetos release; PortOamSetSource, Preliminar130Shadow,
Preliminar130Pixel yDrawObjs conservan SHAexacto. El guard nativo no cambia.
Dos runs finales604 (Wide/Original) PASS. A600 cambian exactamente1904
píxeles físicos (476nativos a escala2); todo el resto de Wide, incluido
escritorio/actores, es idéntico al before2935. Núcleo GBA idéntico en ambos
modos y viewport completo del juego Original idéntico al control. El baseline
Original contenía icono diagnóstico fuera del juego; no se afirma igualdad
de toda la presentación física. La variante final se verificó a600; los
oráculos206 y secuencia2430..2500 siguen siendo evidencia anterior, con los
cuatro bloques de actor/render conservados por SHA, no nuevos runs fullband.
El montaje público muestra ANTES/SOLUCIÓN reales y detalle completo del
tramo. Se entrega como nueva hija pendiente de2935, sin reescribir previas
ni heredar aprobación. Fuente ROM/SAV/instalación permanecen intactos.
