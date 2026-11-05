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

Pendiente valoración humana. Se conserva la muesca verde original de la
propuesta. No certifica todos los fotogramas, otras plataformas ni cambios en
la instalación/release. ROM/SAV originales y revisiones anteriores intactos.
