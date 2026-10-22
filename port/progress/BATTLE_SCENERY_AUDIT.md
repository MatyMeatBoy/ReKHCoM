# Escenarios de combate — revisión del 18/10/2026

## Contadores vigentes

- **28/28 escenarios distintos inspeccionados inicialmente** en EU, con Sora,
  en 16:9 y 32:9; 56/56 ejecuciones, 168 capturas.
- **1/8 mejoras necesarias terminadas y aceptadas:** Guard Armor.
  Implementación, receta y pruebas en `BATTLE_SCENERY_TECHNIQUE.md`.
- **2/28 escenarios aceptados expresamente por el usuario para conservar:**
  Hook e interior normal de Monstro. No equivalen a 2 correcciones nuevas.
- 8 ejecuciones largas adicionales, 24 capturas, para Hook, Jafar, Oogie y
  Marluxia final (fotogramas 900, 1500 y 2100).

## Criterio del usuario

El 18/10 el usuario indicó que Hook y el interior de Monstro ya se ven bien.
En Alicia/Trickmaster, conservar el escenario actual y **continuar hacia abajo
el patrón de ajedrez para cubrir la franja negra**. No redibujar la habitación
entera ni considerar defectuoso cualquier elemento que se repita.

Usar tiles originales y/o reconstrucción de píxeles con continuidad del suelo,
perspectiva, paleta y silueta. No estirar la última fila como sustituto del
dibujo. Separar el escenario de las capas que dibujan al jefe: extender el
fondo no debe repetir a Úrsula, al dragón o a Marluxia. Los huecos que pertenecen
a la silueta del mapa no se rellenan automáticamente.

## Lista de mejoras necesarias — 1/8

| # | Pelea / escenario | Evidencia observada | Trabajo pendiente | Estado |
|---|---|---|---|---|
| 1 | Guard Armor | Recorte rectangular del patio; bloques negros laterales, y copias del patio separadas en 32:9. | Continuación integrada y aceptada; 13/13 pruebas de píxeles. | Terminada |
| 2 | Darkside | Escenario en una ventana estrecha; negro alrededor, partes del jefe cortadas y repetidas en 32:9. | Hecho 22/10, espera aprobación (`KHCOM_DARKSIDE_BEACH=1`): playa por bandas de su patrón base (cielo 1, mar 8, arena 4 tiles; la orilla por columnas limpias), arena debajo; humo (BG2) una sola copia; cuerpo validado aunque su mapa termine (antes se cortaba en el borde GBA). Mitad derecha de Darkside (no existe en el cartucho), dibujada en las 7 poses: de pie (cuadros 0-7) desde la referencia del usuario (`gen_darkside_body.py`); las otras 6 desde ese modelo terminado (`gen_darkside_pose.py`): agachado (36-40) y brazo en alto (41-45) con su hombro, brazo y puño donde coinciden los ojos; garra (9/11) con el pelo del modelo y su propio hombro y garras en espejo; inclinado (8/12) y golpe (13-17, 21-32, hundiéndose) con el hombro del modelo sobre la espalda y el brazo detrás; alcance (10) con el modelo entero en la raíz del brazo. Píxeles del cuadro leídos de VRAM (tiles >511 en el bloque siguiente). Evento 180 (Riku) incluido; solo combate 198 y evento 180 cargan este fondo. También con `KHCOM_PRELIMINAR_V1`. `test_darkside_backdrop.py` 16/16 (con dos casos de ataques). | Propuesta |
| 3 | Oogie Boogie | La sala/ruleta acaba en bordes rectos; negro lateral y copias parciales de la sala en ultrawide. | Continuar paredes/suelo sin duplicar la ruleta, puerta, rejas ni al jefe. | Pendiente |
| 4 | Jafar | Lava, muros y pilares quedan en un rectángulo aislado; al mover la cámara aparece negro en varios lados. | Continuar lava y paredes, respetando los tres pilares móviles y sus límites jugables. | Pendiente |
| 5 | Maléfica dragón | El salón tiene cortes; el cuerpo del dragón aparece repetido en 32:9. | Continuar columnas/baldosas y evitar el wrap de la capa del dragón. | Pendiente |
| 6 | Úrsula | Aparecen dos Úrsulas en la captura 16:9 al mover a la izquierda; también se repite en 32:9. | Extender caverna y suelo, manteniendo una sola Úrsula y un solo tridente. | Pendiente |
| 7 | Marluxia final | La plataforma/jefe y sus efectos se cortan; en 32:9 reaparecen fragmentos en los laterales. | Separar plataforma/jefe de las extensiones del fondo; revisar los modos horizonte/túnel/estrellas y sus transiciones. | Pendiente |
| 8 | Trickmaster (Alicia) | El tablero termina antes del borde inferior y deja una banda negra. | **Solo prolongar el ajedrez hacia abajo**, respetando perspectiva y alternancia. Conservar la habitación actual. | Pendiente |

La lista cuenta escenarios, no el número de veces que aparece cada jefe en
Sora/Reverse-Rebirth. Estos ocho escenarios afectan a sus respectivas peleas
cuando se usan en ambas campañas. La verificación realizada aquí es EU/Sora;
la validación regional y con Riku se hará al implementar cada corrección.

## Inventario completo — 28 escenarios

«Sin bloqueo en muestras» significa que no se añade un redibujado a esta lista
a partir de estas capturas; no equivale a aprobación final de todos sus modos.

| Escenario | Peleas que lo utilizan / cobertura | Resultado inicial |
|---|---|---|
| Twilight Town | Combates de campo de Villa Crepúsculo | Sin bloqueo en muestras; conservar borde inferior hasta revisión específica. |
| Traverse Town | Campo de Ciudad de Paso; Leon/tutoriales | Sin bloqueo en muestras; franja inferior plana anotada, sin rellenarla automáticamente. |
| Wonderland | Campo del País de las Maravillas | Sin bloqueo en muestras; no confundir con el tablero de Trickmaster. |
| Olympus Coliseum | Campo y Cloud | Sin bloqueo en muestras. |
| Monstro | Combates normales del interior de Monstro y encuentro de sombras | **Conservar: usuario indica que ya se ve bien.** |
| Never Land | Campo del barco, barriles | Sin bloqueo en muestras; distinto de la cubierta de Hook. |
| Halloween Town | Campo y encuentro de historia | Sin bloqueo en muestras. |
| Agrabah | Campo y encuentros de historia | Sin bloqueo en muestras; distinto de la sala de Jafar. |
| Atlantica | Campo de Atlántica | Sin bloqueo en muestras; mantener la silueta del terreno. |
| Hollow Bastion | Campo de Bastión Hueco, encuentros de Riku en ese mundo | Sin bloqueo en muestras; distinto del salón del dragón. |
| Destiny Islands | Campo de Islas del Destino | Sin bloqueo en muestras; distinto de Darkside. |
| Castle Oblivion | Campo del Castillo del Olvido | Sin bloqueo en muestras; distinto de los duelos humanos. |
| Garden | Soldados naipe / jardín | Sin bloqueo en muestras; mantener silueta del césped. |
| Hades | Hades | Sin bloqueo en muestras; su arena difiere de Cloud. |
| Hook | Capitán Hook, incluida cubierta inclinada hasta frame 2100 | **Conservar: usuario indica que ya se ve bien.** |
| Mansion | Vexen 2 y Riku 6 | Sin bloqueo en muestras; ambas peleas comparten el exterior de la mansión. |
| Ansem 2 | Ansem final de Reverse/Rebirth | Sin bloqueo en muestras iniciales; esta captura usa el fixture de Sora, no una partida completa de Riku. |
| Marluxia | Primer duelo con Marluxia | Sin bloqueo en muestras; no confundir con Marluxia final. |
| Hum | Riku 1–5, Axel 1–2, Larxene 1–2, Vexen 1/3, Ansem 1 y Lexaeus | Sin bloqueo en muestras del fondo compartido; representante Axel 1. |
| Parasite Cage | Jaula Parásita, plataformas y ácido | Sin fallo bloqueante confirmado en muestras; no se propone redibujar Monstro por defecto. Validar cambios de plataforma durante la futura revisión de animaciones. |
| Darkside | Darkside | Mejora #2 pendiente. |
| Guard Armor | Guard Armor | Mejora #1 pendiente. |
| Trickmaster | Trickmaster | Mejora #8: prolongar solo el ajedrez inferior. |
| Oogie Boogie | Oogie Boogie | Mejora #3 pendiente. |
| Maléfica dragón | Dragón Maléfica | Mejora #5 pendiente. |
| Úrsula | Úrsula | Mejora #6 pendiente. |
| Jafar | Jafar genio | Mejora #4 pendiente. |
| Marluxia final | Segunda forma / plataforma | Mejora #7 pendiente. |

## Evidencia y reproducción

Build sin modificaciones de render: `ee753f7c1`, Windows EU.
SHA-256 de `khcom.exe`:
`e7494492d12fa0bb300715f70406dcaa838cc3d73db67ca203a08dfd3c315475`.

Herramienta: `port/tools/audit_battle_scenery.py`. Inspección de las rutas
`src/btl/btl_map.c`, `src/btl/mode_battle.c`, `src/bos/bos_map.c` y los mapas
específicos de cada jefe; inventario del menú debug en
`src/debug/mode_chkbtl.c` (209 entradas que comparten estos fondos).

```powershell
python port/tools/audit_battle_scenery.py --build build/codex-evidence/gated-eu --out build/codex-evidence/battle-scenery-audit-20261018 --jobs 2
python port/tools/audit_battle_scenery.py --build build/codex-evidence/gated-eu --out build/codex-evidence/battle-scenery-long-20261018 --only HOOK,JAFAR,OOGIE_BOOGIE,MARLUXIA_2 --frames 2110 --shots 900 1500 2100 --jobs 2
```

Cada salida es una carpeta nueva dentro de `build/`, con `index.html`, PNG,
hojas de contacto y `report.json`. La primera toma frame 180 sin movimiento
del jugador, 380 tras mover a la izquierda y 600 tras mover a la derecha.
El combate sigue activo: esto no garantiza que cada cámara llegue al límite
geométrico ni que se cubran todos los ataques o fases.

16:9 usa 960×540 a x2 (lienzo interno equivalente a 1920×1080 a x4).
32:9 usa 960×270 a x1 (equivalente a 3840×1080 a x4). Audio/vídeo dummy,
directorios aislados, sin tocar partidas ni instalaciones del usuario.

No son 209 combates completos ni una certificación regional. Falta comprobar
las fases no vistas de Marluxia final, cámara vertical, entradas/salidas,
zoom/otros aspectos y Riku/EU/US/JP al implementar las correcciones. Cerrar una
mejora exige sus capturas antes/después y revisión visual, no solo exit=0.

Las capturas locales contienen recursos del cartucho del usuario: no se
incluyen en el commit ni se suben a GitHub.
