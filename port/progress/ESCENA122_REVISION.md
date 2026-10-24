# Coliseo 122/123 — revisión de la geometría, 24/10/2026

Estado: propuesta corregida y comprobada, pendiente del usuario. Revisor: Codex.
Base: `4cf65effb` más el borrador local recuperado, preservado en stash. Ese
borrador confundía pared y suelo y se descarta como base visual. Main actual
se incorporó mediante `effaf1acb`, conservando las escenas avanzadas de la rama.

## P0/P1 — identidad y referencias

`gEventBg122`, eventos 122 y 123. Mapas de 1024×1024, dibujo 440×336
(55×42 tiles). Las tres regiones comparten hashes BG3 `2520775749` y
BG2 `1445205069`. BG3 contiene mampostería y pavimento; BG2, friso,
relieves y variaciones del pavimento. Los personajes y columnas son OAM.

Referencias abiertas: Agrabah 107 EU 3440×1440, Agrabah 109 US 1920×1080
y Guard Armor 32:9, bajo `build/codex-evidence`. De Agrabah se conserva el
método de continuar el borde entero antes del detalle, con objetos completos.
De Guard Armor, la separación entre escenario y actores. No se copian sus
coordenadas. Exportaciones originales por capa y rejilla en
`build/preliminar-v1/resume-122/bg{2,3}[-grid].png`.

## P2/P3 — planos y donantes

Todas las coordenadas siguientes son píxeles del mapa, sin ampliar.

| Zona | Límite original | Donante y transformación |
|---|---|---|
| Muro izquierdo | x<184; contacto BG3 `2*y-x=-24` | BG3 x=128..159; `sy=y+(sx-x)/2`; módulo vertical de 32 px arriba |
| Frente central | x=184..311; base y=96 | Misma x; módulo vertical de mampostería de 64 px, sin repetir el relieve |
| Muro derecho | x≥312; contacto BG3 `2*y-x=-152` | BG3 x=320..351; misma traslación diagonal; módulo vertical de 32 px arriba |
| Friso izquierdo | En x=24: y=1..16; en x=40: y=9..24 | BG2 x=24..39, y=0..31; píxeles con fase de 4 px; excluye relieves |
| Friso central | y=80..95 | Original intacto, sin prolongarlo horizontalmente sobre las paredes laterales |
| Friso derecho | En x=384: y=117..132; en x=392: y=121..136 | BG2 x=384..399, y=112..143; traslación diagonal, sin relieves |
| Pavimento | Debajo de cada contacto | BG3 x=0..63, y=160..223; BG2 x=0..127, y=160..287; módulos completos sin objetos |

El primer ensayo usaba x=0..15 para el friso izquierdo: esa tira está cortada
por y=0 y pierde parte del grosor. La revisión la cambia a x=24..39 antes del
cierre. Se conserva el contraste y grosor de cada pared y su retorno central.

Los relieves originales cortados por y=0 se completan usando el panel intacto
de BG2 x=312..375, y=16..111: para x=56..119, desplazamiento fuente (+256,+64);
para x=120..183, (+192,+32). La parte original de ambos paneles coincide con
el donante salvo 12 píxeles en el rectángulo comparado (variaciones existentes
del mapa que se conservan). El panel de x=408..471, cortado por x=440,
coincide exactamente con el donante desplazado (-96,-48). Solo se completa
la parte ausente: no se agregan relieves en otras posiciones.

## P4/P5 — revisión visual

Implementación: `PortPreliminarPixelSource`, llamada por el PPU únicamente en
los márgenes. Muestreo de índices originales y paleta activa. Se mantiene el
muestreo por tiles para los demás fondos. Activación con
`KHCOM_PRELIMINAR_V1=1`; antes individual con `KHCOM_PRELIMINAR_SKIP=122`.

Ensayo `quick-v2/escena-122/3440x1440-300-limpio.png` abierto a x3 y en
recortes a 1:1, en el directorio privado del estudio:

| Recorte x3 | Caja en la captura x3 | Observación |
|---|---|---|
| left-frieze | (1000,220,1400,600) | La diagonal continúa; la tira inicial truncada requiere la corrección de P3 |
| upper-reliefs | (1200,100,1740,600) | Paneles superiores completos, sin añadir posiciones de relieves |
| central-return | (1620,300,2220,740) | Dos quiebres, frente central y cornisa en su lugar |
| right-relief | (2320,500,2690,1020) | Se completa el panel derecho que cortaba el mapa |
| lower-floor | (1040,1080,1800,1430) | Pavimento con módulos completos; columnas únicas conservadas |

Evidencia final abierta: `eu-final/escena-122/3440x1440-100-despues.png`,
`3440x1440-300-limpio.png`, `3840x1080-300-despues.png` y
`eu-final/evento-123-escena-122/3440x1440-1000-despues.png`. Recortes finales
`*-final-x3.png` y `*-final-native.png`, mismas cajas de la tabla: friso entero
en ambas paridades, retornos intactos, paneles completos y objetos únicos. El
estado 100 muestra además el fundido de la paleta activa. Sin defectos
estructurales detectados en estas muestras; no equivale a aprobación.

## P6/P7 — cierre

- EU amplia: 24 pares (12 por evento), cuatro ventanas, estados 100/300/1000.
- Original EU: 24 pares, imagen completa idéntica.
- US y JP: dos pares amplios y dos Original por región; muestras rápidas
  3440×1440, estado 300, ambos eventos. No son matrices regionales completas.
- Ocho controles EU: 004/057/061/074/107/109/112/121, imagen completa idéntica
  al apagar solamente 122. `check_sources.py`: correcto.
- Todas las comparaciones conservan el área GBA. Informes finales en
  `resume-122/eu-final`, `original-eu-final`, `controls-final`, `us`,
  `original-us`, `jp`, `original-jp`. Los directorios EU sin sufijo `final`
  son ensayos anteriores, no la evidencia de entrega.

SHA256 del ejecutable EU final: `841bac3e5133454395afb62371a24ce2c9ed536eb3c345e18e3af037de7aadcb`.
`resume-122/manifest.json` vincula cada informe con el SHA256 de su ejecutable
copiado; se comprobó que coincide con la compilación regional final. Evidencia
promovida a `ready/escena-122` y `ready/evento-123-escena-122`. Copia jugable:
`../pruebas/ReKHCoM-preliminarV1`, con antes/después de ambos eventos.

Los estados cubren entrada/fundido, diálogos y cambios de posición alcanzados
por el arnés. No certifican todas las cámaras, animaciones ni la salida completa
del evento. Ninguna aprobación del usuario se presume. Siguiente: reabrir 121
por friso, fuste, remates y mampostería; después 124, 129, 131 y 132.
