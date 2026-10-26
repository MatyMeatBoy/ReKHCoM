# Ver barajas: sincronía de la extensión de pestañas

Las filas originales se dibujan antes de VBlank; la presentación de márgenes sucede después de DMA.
PortMenuTabPixel leía VRAM y paleta ya actualizadas para el siguiente cuadro, mezclando dos cuadros.
Se guardan los 16 colores de cada extremo BG al terminar el render, antes de VBlank; Present usa esa copia.
Las pestañas hechas de sprites conservan su ruta. No cambia arte, navegación ni dibujo original.

Reproducción aislando mandos externos: entrar Mode_Deck, subir al filtro, derecha y volver izquierda.
42 fotogramas EU a1536x864; solo cambia la extensión izquierda del cuadro661 (x0..119, y97..172).
71 filas discrepantes en la unión pasan a0; región original idéntica en los42 y estado estable idéntico.
Comando reproducible: test_menu_tab_sync.py --reference <build-anterior> --build build/pc-eu.
Build EU y check_sources pasan. Claude integra. El parpadeo HD de Añadir naipes es otra tarea.

## Primera pestaña del ojo: fundido de entrada y salida (26/10)

El desfase leve no era parpadeo ni el latch de PR84. La limpieza del trazo aislado del ojo, necesaria en el extremo
**derecho** de la colección, también se aplicaba a la izquierda. En los cuadros más oscuros del fundido el cuerpo
de la pestaña y el panel se redondeaban al mismo negro; esa comparación borraba el borde superior legítimo de la
primera pestaña hasta que el cuerpo empezaba a iluminarse.

La corrección limita esa limpieza al extremo derecho. No cambia animaciones, posiciones, colores, recortes, fondo
ni difuminado inferior. Reproducción EU con ROM propia, sin mandos externos:

```
python port/tools/test_deck_eye_timing.py --build build/pc-eu --reference <build-anterior> --out <work>/deck-eye --window 1920x1080
python port/tools/test_deck_eye_timing.py --build build/pc-eu --reference <build-anterior> --out <work>/deck-eye-hd --window 3840x1080 --hd 1 --sprites <mod-importado>/sprites
```

La prueba recorre entrada y salida, compara el borde original con su extensión en cada cuadro y, con referencia,
exige que el resto de la imagen sea idéntico. Conserva PNG sin pérdida, log, INI y SHA256 del ejecutable; los BMP
que acaba de generar se sustituyen por esos PNG para no acumular copias.

- Regresión antes/después, HD apagado y encendido, 1920/2560/3840x1080: cuatro cuadros con desfase pasan a cero.
  Solo cambian 212–214 y 339; sus diferencias están dentro del margen izquierdo de la pestaña.
- En el recorrido natural START → Ver barajas → B, los mismos cuatro cambios aparecen en 334–336 y 639.
  El rectángulo GBA, el guante, el encaje y las filas bajo la pestaña permanecen idénticos.
- La comprobación de Moguri conserva sus tres pantallas y el fundido inferior; no se ha redibujado arte.

La evidencia completa de esta pasada está en `<work>/deck-sync/`: `green-matrix.json`, `matrix-compare.json`,
`matrix/manifest.json` y `matrix-after/manifest.json`. No se cambió la valoración de ningún dibujo de Atlas.
