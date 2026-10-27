# OptiPort — batería ReKHCoM 2.1

Skill pública y reutilizable: [MatyMeatBoy/OptiPort](https://github.com/MatyMeatBoy/OptiPort). La copia en `port/skills/optiport` identifica el método aplicado en esta revisión; los tickets y resultados del juego se mantienen aquí, no en la skill general.

## Avance comprobable

**10 de 14 bloques originales integrados: aproximadamente 71 % del trabajo de
la primera batería**, no porcentaje de estabilidad ni garantía de bugs eliminados.
Snapshot de coordinación del 27/10/2026; reservas y estados vivos en
`coordination/art-queue`. El hito previo 9/14 (~64 %) se conserva como referencia
histórica. La migración 32→64, la matriz de plataformas y los nuevos controles,
galería y previews son alcance añadido fuera de este denominador. No convertir
ese 71 % en avance de todo el proyecto.

| Bloque original | Estado y evidencia |
| --- | --- |
| Logs y diagnóstico | Integrado #282/#298: actual + cinco anteriores y fecha UTC; conservar evidencia de fallos de rotación. |
| Portabilidad/alineación | Integrados #286/#318/#319: alineación ROM y lecturas BIOS en puerta/carga guardada. El caso privado real sigue siendo regresión obligatoria del artefacto final. |
| Pools/paletas | Integrado #285/#286: agotamiento inyectado, mensajes, elecciones y cartas. |
| Imágenes y presupuesto | Integrado #284/#296: cabeceras antes de decode, límites/pin/LRU y presupuesto de targets HUD. |
| Readiness del HUD | Integrado #283/#287/#297: bounds, grupos requeridos e invalidación explícita del HUD. |
| Música HD | Integrado #288/#291/#295: progreso acotado, host bridge y límites del loader. |
| Vídeo HD | Integrado #289/#293: primera textura/upload, fallos MF y lectura/cancelación Linux. |
| Profiling | Parcial: arnés serial, cold/warm, hashes, tiempos y RSS/private. Completar escenarios representativos; memoria de proceso no equivale a VRAM. |
| Asíncrono/precarga | Parcial: medir entrada/caché y auditar cancelación/resultados obsoletos antes de introducir trabajo adicional. Previews de ambos modos son ampliación separada. |
| Streaming/backpressure | Integrado #293/#291: pipes Linux y alimentación musical host con progreso/cancelación acotados. |
| Frame pacing | Parcial: quedan mediciones p95/p99 y colas en escenarios apropiados, conservando velocidad GBA. |
| Lifecycle | Integrado #309/#311: generaciones bridge e invalidación de recursos nativos en device loss; hardware real y pausas visuales se documentan aparte. |
| Protección del progreso | Integrado #290/#314: preservación de SRAM ante flush fallido y estados atómicos. No promete recuperación automática de memoria corrupta. |
| Entrega y resistencia | Parcial: paquete privado y repro guardado pasan 32+64; faltan cierre de evidencia, soak dirigido y entrega pública/confirmación de reporteros. |

No se exige completar una campaña entera: usar fixtures deterministas por patrón
y soak dirigido que cubran los riesgos de cada cambio. Mantener el caso real
que descubrió el fallo junto a las pruebas sintéticas.

## Snapshot vigente de artefactos — 27/10, fuentes e71c9692c

- El preview anterior falló con la SRAM real en la primera puerta (AV WER
  `0x100f7bd6`, `MapWallFaceIsUnreserved`, fila superior NULL/lectura BIOS GBA).
  La corrección integrada #318 alcanzó `field=520`, salida 0.
- Windows e71: ZIP extraído, seis regresiones PASS (guardado privado, ocho salas
  Sora/Riku y puerta de evento, cada una en 32 y 64). Reporte privado
  `build/pre1-package-regressions-e71/report.json` en el checkout de diagnóstico.
  El ZIP limpio está en la raíz KHCoMPC, SHA-256
  `3645fe9013125be6722e28beb39e991ab27d874725044aa739a9dee2bdfb82a4`.
- Linux e71: paquete desktop/Deck x86+x64 compilado y probado en VM Debian;
  ambos runtimes llegan al campo del guardado y a la batalla de Traverse Town.
  La primera corrida de puerta de evento tuvo un error de preparación del
  harness, conservado y repetido correctamente; no fue un fallo del juego.
- Android e71: APK universal construido; payload nativo del APK jugable idéntico
  al smoke en las bibliotecas/imágenes del juego, salvo launcher de prueba.
  El launcher jugable arranca en emulador API34 con traducción ARM64. La primera
  prueba del guardado terminó con salida0 sin alcanzar postcondición: logcat
  muestra recreación de Activity por CONFIG_ASSETS_PATHS tras instalar. Se
  repitió tras estabilizar el entorno: probe de tres checkpoints y guardado real
  PASS. La puerta de evento y Monstruo siguen en curso. El intento anterior no
  cuenta como PASS ni como crash nativo. No representa el Samsung Android13 del reportero.
- GPU Windows e71: marcos de stocks reales 1/2/3 de Sora y Axel visibles;
  bordes Moguri/Estado iguales al fondo al entrar en frío y volver. Evidencia
  privada `build/pre1-stock-e71` y `build/pre1-visual-e71`. La equivalencia de
  vida/capacidad HD y el AA portable siguen pendientes.
- No hay prerelease pública nueva en este snapshot. Un defecto de vida útil de
  valores `getenv` en el runtime portable se reprodujo durante la revisión;
  #343 integrado `34132bd71`. Reconstruir y repetir artefactos afectados antes
  de publicar; no atribuirle el cierre previo de Activity sin evidencia causal.

## Integración y siguientes pruebas mínimas

| Trabajo | Estado verificable / siguiente paso |
| --- | --- |
| Marcos de stocks, HP enemigo, entrada de submenús | #320/#323/#324/#325/#328/#326 integrados; capturas GPU anteriores, no prometer paridad visual total. |
| Cartas pequeñas centradas | #315 integrado `15f84f425`; hoja real de 105 revisada y 448 recetas válidas. Falta comparación en runtime actualizado. |
| Harness de acordes | #333 integrado `4452d0a23`; el parser anterior sólo pulsaba el primer botón de L+R. No reutilizar pruebas de acordes antiguas como cobertura efectiva de stocks. |
| Fuente Steam EN/JP | #327 integrado `7297351f1`; señal de procedencia acotada y sólo lectura. No certifica audio ni cambia importaciones legacy. |
| Deck Gaming Mode | #337 integrado `c8109de7d`; asistente de registro/guía, 10 pruebas wrapper +3 paquete. Falta Deck real; #330 no demuestra fallo de Gamescope. |
| Targets GPU portable | #331 en borrador, dos P1 reproducidos/corregidos y revisión independiente; faltan GPU Windows/Linux/Android. |
| Controles | #332/#334/#338/#340 en revisión. Faltan QA visual, activar Original→sí en diálogo abierto y casos de texto variable. |
| Biblioteca EN/JP | #335 y #342 apilado en revisión; P1 de manifiesto mixto reproducido por revisor, no integrar hasta corrección/regresión. |
| Retorno de galería | #339 en revisión; productor/consumidor RED→GREEN, matriz de capturas título/partida × normal/skip pendiente. |
| Dígitos azul/dorado | #341 en borrador: conserva GBA multicolor si falta el dígito HD; fuente real EU/US/JP pasa, comparación GPU pendiente. |
| Linux ARM64 | Factibilidad local `9300d2992` identificada; revisar/actualizar y construir, no confundir con armhf ni con Deck x86-64. |

La tabla registra el estado de esta fotografía, no reemplaza claims ni estados
remotos. El 71% anterior sigue limitado a los catorce bloques originales;
profiling, asíncrono, pacing y resistencia continúan parciales.

## Reportes y límites

- #277: fixture correcto de puerta de evento, dos fallos reproducidos (alineación y paleta), corregidos. Entrada hasta batalla frame 2239 y salida 0 en Windows/Linux64. Falta Deck del reportero. [Detalle](DOOR_PORTABILITY_277.md).
- #272: evento 74 USA pasa en Linux con HD apagado; no reproducción Android aún. Mensajes/paletas protegidos; no afirmar causa resuelta.
- Cierre propio en Monstruo al volver a ventana: no reproducido ni atribuido a RAM. Instalación Quiver **sólo lectura; jamás ejecutar ni modificar allí**. Los tests usan copias aisladas.
- No publicar estable 2.1. El usuario autorizó integrar y enviar builds de prueba a los reporteros; mantener claramente separadas las pruebas de la release estable y no incluir ROMs/partidas/configuraciones privadas.

El corpus local de SRAM contiene **un caso único**; no describirlo como una
colección amplia de partidas. GameFAQs es un catálogo candidato de saves
públicos: no descargados ni validados, acceso/obtención bloqueado por ahora.
No contar esos candidatos como cobertura ni distribuir ROMs/datos privados.

Cada entrega debe actualizar PR, SHA, pruebas y pendientes. Seguir implementando y subiendo bloques mientras continúa la investigación; no esperar al final de la batería para entregar.
