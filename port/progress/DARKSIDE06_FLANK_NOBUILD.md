# Darkside06: propuesta de flanco estrecho y sombra local, sin build

Padre Atlas `60f90456598249ecb1e691c51dd2434e`, versión r8, revisión2
approved, SHA256 `686a6576e4ee1cc085f4fffb10fc09c36954f90fcebefb739a6696e99a26a3ad`.
Envío `20261025-130635-c0c34f`, nube
`72f6d65b-36e4-47b2-8f2c-d94a9f2ec26d`: «falta crearle dibujo+ difuminado
a este lado». La aceptación del padre no aprueba automáticamente esta hija.

La nueva pieza sólo propone una sombra curva estrecha entre torso y brazo.
Se descartó el antiguo peine y, en el estudio privado, un contorno ancho con
un lóbulo azulado. La propuesta revisable usa211píxeles de índices1..4, ancho
máximo8, sin añadidos en y>=180. El taper inferior nativo y el brazo aprobado
de1842píxeles permanecen intactos. Las filas añadidas son contiguas y empiezan
en la costura x256; sólo el píxel fraccional exterior usa difusión2D estable.
No hay Bayer, bandas horizontales de cobertura ni partículas aisladas.

Fuente desde main27ad033d en un checkout nuevo, sin transferir el WIP anterior.
Generador y matriz propios separados del dibujo aprobado. Se leen tiles y
paleta de la ROM local en sólo lectura; el repositorio guarda exclusivamente
la nueva continuación 4bpp, sin RGB fijo ni referencias nativas publicadas.

El helper nuevo es opt-in `KHCOM_DARKSIDE06_FLANK=1`: ausencia,0 y otros valores
no lo activan. El PPU lo solicita sólo para la capa corporal identificada,
fuera del núcleo GBA240x160 y después del dibujo aprobado. El helper verifica
puntero/alineación de slot y la única copia cercana. Sólo selecciona13 y21..28,
con desplazamientos de la costura nativa comprobados. Usa paleta viva del
fotograma. Los cuatro helpers anteriores y los dibujos aprobados no cambian.
Esta opción pendiente no se activa por aceptar otra pieza.

Pruebas sin compilador:3unittest de datos; generador reproduce exactamente la
matriz;126400casos interpretados del selector C real y1097predicados C de
opt-in, puntero, límites y caller. Las nueve costuras nativas y los1842píxeles
del brazo se conservan; los nueve archivos del WIP original siguen con sus
SHA256. Se comprobó la ausencia de añadidos en la terminación inferior.

La comparativa es un **estudio offline de la pose13** al mismo encuadre y
escala, construido con ROM y dibujo aprobado. No es captura del juego ni
demuestra movimiento/oclusiones, integración de la cámara o rasterización.
El brazo estático mientras desciende el cuerpo sigue siendo una limitación
preexistente; esta sombra no la resuelve. No se declara cuerpo terminado.

SIN BUILD/COMPILADOR/JUEGO/IMPORTS. No se ejecutaron fixtures C ni scripts
de captura/build, importador, paquetes o releases. No se cambiaron ROMs,
partidas, instalaciones, datos humanos de Atlas ni otros WIP. Pendientes:
valoración de la hija y QA real de Original/cámaras/plataformas cuando se
autoricen builds. Evidencia privada:
`pruebas/atlas-20261106/darkside06-polish/`.
