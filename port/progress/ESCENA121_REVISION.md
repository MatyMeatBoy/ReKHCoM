# Coliseo 121 — revisión reabierta, 24/10/2026

Estado: propuesta corregida y comprobada, pendiente del usuario.
Revisor: Codex. Sustituye la propuesta visual de `4cf65effb`, que pasaba las
comparaciones nativas pero repetía una fila de mampostería, perdía el friso y
cortaba la columna en y=0. Se conserva como evidencia histórica, no como base
visual válida. La 122/123 de `c62c7d8ee` permanece congelada.

## P0/P1 — fuentes y base

`gEventBg121`, solo evento 121 en la tabla de fondos. Almacenamiento 512×256;
BG3 dibujado 384×256, BG2 368×152. Hashes regionales iguales: BG3 `1315845753`,
BG2 `937644977`. BG3 contiene pared/pavimento; BG2, friso, emblema, tres
ventanas y el fuste de esquina. Personajes, pilares delanteros y bloques son
OAM: no se modifican ni se duplican.

Se abrió el panorama anterior de `ready/escena-121` para contrastar el rechazo,
además de las capas completas y sus rejillas en `resume-121/bg{2,3}[-grid].png`.
Referencias del procedimiento: Agrabah 107/109 y Guard Armor, abiertas durante
esta tanda; borde entero, objetos íntegros y muestreo de paleta activa. Se
traslada el método, sin reutilizar sus coordenadas.

## P2/P3 — geometría y donantes

Coordenadas en píxeles de fondo, sin ampliar. El friso tapa el escalonado del
contacto por tiles de BG3: debe continuar con ambas paridades a nivel de píxel.

| Plano/remate | Ecuación o posición | Donante íntegro |
|---|---|---|
| Pared izquierda | x<192; contacto `2*y+x=272` | BG3 x=0..31, y=0..63; `sy=y+(x-sx)/2`; módulo de altura 64 px |
| Retorno oscuro | x=192..223; contacto `2*y-x=-112` | Misma x, módulo vertical BG3 de 32 px; conservar su sombreado propio |
| Pared derecha | x≥224; contacto `2*y+x=336` | BG3 x=224..255, y=0..31; traslación por su propia diagonal |
| Friso izquierdo | (0,136)..(16,128), grosor 16 px | BG2 x=0..15, y=128..159; píxeles y desfase de 4 px |
| Friso derecho | (240,48)..(248,44), grosor 16 px | BG2 x=240..255, y=40..63; excluye columna y ventanas |
| Fuste de esquina | x=214..233, cruza y=0 | Misma x, BG2 y=0..31; se prolonga verticalmente, sin columna nueva |
| Ventanas cortadas | x=284..291 y 304..311 | Ventana original intacta x=264..271, y=0..25; desplazamientos fuente (-20,+10) y (-40,+20) |
| Pavimento | Debajo de cada plano | BG3 x=0..127, y=160..223; módulo completo, sin columnas ni bloques |

El primer ensayo repetía también el desgaste gris de x=32..63. Se cambió al
donante limpio x=0..31 para conservar únicamente las manchas originales. El
emblema de x≈80..151 está completo y no se repite. No se coloca una cuarta
ventana fuera del mapa: solo se reconstruyen las dos visibles que cruzan y=0.

## P4/P5 — revisión

Implementación en `PortPreliminarPixelSource`, solo en márgenes; mismo selector
V1 y `KHCOM_PRELIMINAR_SKIP=121` para aislar el antes/después. BG3 mantiene
módulos de mampostería completos: se elimina el muestreo fijo de una fila.

Abierta `quick-v2/escena-121/3440x1440-300-despues.png`: las diagonales llegan
continuas, se conserva el retorno oscuro, la columna sale por arriba con el
mismo ancho y las tres ventanas son enteras. Emblema, actores y bloques únicos.
Panorama final 3840×1080 abierto: continúa el friso en ambos extremos, sin
rayas ni cortes de fuste. Muestra JP 3440×1440 abierta, misma geometría.
Recortes abiertos a 1:1 y x3 (coordenadas del lienzo x3 3440×1440):

| Recorte | Caja | Veredicto |
|---|---|---|
| left-frieze | (1040,620,1320,1060) | Grosor y ambas paridades conservados |
| upper-column | (1660,160,1950,580) | Mismo fuste, sin corte en y=0 |
| windows | (1870,160,2190,480) | Tres ventanas enteras; sin cuarta ventana |
| right-frieze | (2140,150,2420,430) | Diagonal completa hacia arriba/derecha |
| return | (1620,340,1900,620) | Retorno oscuro y pie del pilar íntegros |
| floor | (1300,950,1700,1260) | Pavimento completo, sin banda horizontal |

Archivos `*-native.png` y `*-x3.png` en `resume-121`, de `quick-v2`, mismo
binario final. Solapamiento de las ventanas donantes con el dibujo original:
una variación de un píxel en la segunda, coincidencia exacta en la tercera;
las partes existentes no se reemplazan. Sin defectos estructurales detectados
en estas muestras. La revisión del agente no implica aprobación del usuario.

## P6/P7 — cierre

EU: 12 pares amplios y 12 Original, cuatro ventanas, estados 100/300/1000.
US/JP: una muestra amplia y una Original por región a 3440×1440, estado 300;
no son matrices regionales completas. Ocho controles EU
004/057/061/074/107/109/112/122: imagen entera idéntica al desactivar solo 121.
Cero diferencias GBA; Original idéntico completo. `check_sources.py`: correcto.

SHA256 del ejecutable EU final: `ac0e7f8cab61da6dc3844b7b874148bbb484d98e4fc17c796ea6007b50b44c88`.
`resume-121/manifest.json` enlaza los informes con el SHA256 comprobado de
cada ejecutable regional. Finales: `eu-final`, `original-eu-final`,
`controls-final`, `us`, `original-us`, `jp`, `original-jp`. La evidencia
rechazada se conserva en `rejected-4cf65effb`; la corregida pasa a
`ready/escena-121` y a la copia jugable Preliminar V1.

Se comprobaron entrada/fundido y estados de diálogo alcanzados por el arnés.
No todas las cámaras, mosaicos, animaciones o la salida completa del evento.
No se modifica lógica, cámara, OAM ni las propuestas anteriores. Siguiente:
124/126; quedan también 129/151, 131/153 y 132/133/154.
