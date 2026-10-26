# Baseline del tráiler ReKHCoM v2.0 — 26/10/2026

Baseline ASCII y Rebel limpio exportado y verificado: `../arte/Trailer/v20/ReKHCoM_v2.0_trailer_final.mp4`
(ruta desde la raíz del repositorio). **No se ha publicado online. No es todavía la entrega definitiva:** el usuario pidió corregir también la recarga HD mediante decompilación. Falta incorporar el binario corregido y una comparativa real de llenado/soltar/retomar/ciclos.

El baseline y sus metadatos se conservan en `baseline_ascii_rebel/`; el vídeo tiene SHA-256 `f1497f78ae191fcd0a05848e5d63e3f65b5c58dd998eda181306a8949c9e8cd4`.

- 1920×1080, H.264, 60 fps, 9714 fotogramas, 161.900000 s (2:41,9).
- AAC estéreo, 48 kHz; audio 161.900000 s. Diferencia de duración A/V: 0.000 s.
- 105,344,948 bytes. SHA-256: `f1497f78ae191fcd0a05848e5d63e3f65b5c58dd998eda181306a8949c9e8cd4`.
- Decodificación completa con `ffmpeg -v error -xerror`: salida 0. Pico PCM decodificado 0.8417, sin clipping.
- 18/18 segmentos y 5/5 actos decodificados también por el carril de QA independiente.
- Contactos finales en `previews/contact_1.jpg`, `contact_2.jpg`, `contact_3.jpg`; miniatura `ReKHCoM_v2.0_thumbnail_final.png` y descripción `YouTube_descripcion.txt` con nueve capítulos.
- Datos reproducibles: `verification.json`, `timeline.json`, `shot_manifest.json`, `root_verification.json`, `atlas_honors_inventory.json` y `heart_mask_equivalence.json`, todos dentro de la entrega privada.

## Procedencia y alcance

Gameplay nuevo de la estable pública v2.0, ejecutable EU sin modificar:
`4d154a7fca1d3bb7627cdd7deb3baf8f6b5f8e8357bfdced9c058363c472cfb2`.
Fuente de la compilación: `59b44df99e05e1f4245516447d5800c88be72cf1`.
Grabación Windows, procesos aislados y ocultos, vídeo/audio dummy, renderer software y mandos físicos desactivados solo en esos procesos. No se tocan partidas instaladas.

Sliders del título HD, Guard Armor, Darkside, Agrabah 109, escena 074 y HUD HD. Ambas mitades comparten entrada, cámara, escala x4 y tiempo de fuente; B comienza recortado al instante del barrido. En seis comprobaciones de Guard Armor/Darkside, el rectángulo GBA central 960×640 conservó **cero píxeles diferentes**: solo cambia la extensión exterior.

Referencias exactas con honores de Atlas, incluidas en el manifest:

| Pieza | ID de la versión con honores | Referencia |
|---|---|---|
| Guard Armor / patio 008 | `7ef20691e77d4bc0b99730ea3443f4af` | Versión con honores · 20/10 |
| Agrabah 109 | `3fa0a50b4582424cbef46d540076c838` | Versión con honores · 21/10 |
| Escena 074 | `d08d6c8758ff4ca9906c89a4c4d95888` | Turbo sin fragmento del ornamento; `c7281ca84` |

Darkside aparece como combate ampliado aprobado: los honores de su cuerpo «De pie» no se atribuyen al contexto de su arena. Jafar tiene una secuencia propia de 24 s por pedido del usuario, sin afirmar honores de su arena.

HUD/cartas/rótulos usan la importación HD actual, incluidas las dimensiones actuales de Guard/CardBreak/Recover; las tomas que usaban el import antiguo fueron reemplazadas. La música GBA y HD compara la misma frase: RMS 0,1 en ambas fuentes; correlación de chroma 0,8737685, desfase óptimo 0 s y escala temporal 1. Se conservan los efectos del combate.

El bloque de cinemáticas es un extracto en movimiento del archivo importado `ROP.mp4`, 7–19 s, rotulado como material importado para Windows/Android; no se presenta como captura de gameplay. El editor es la interfaz real del launcher estable con paneo editorial, sin interacción inventada. El usuario confirmó que el bloque solicitado como «Sincorazón» corresponde al modo Rebelde: trucos y logros locales. La toma de Rebel usa Jafar con HP infinito y daño normal, con una secuencia de entradas diferente a la del bloque principal; no modifica el estado del HUD.

Las tarjetas distinguen Windows/Linux/Steam Deck estables, Android/Anbernic/PS4/Vita/PS5/Web beta y **Nintendo Switch como beta anterior v2.0-pre1**. No hay paquete Switch nuevo en la estable v2.0 ni capturas de PC presentadas como grabaciones de consola.

Texto inglés, Palatino, línea dorada, transiciones de corazón y destellos. La expresión del corazón se simplificó algebraicamente multiplicando por su denominador positivo: 16 máscaras 1080p verificadas, cero píxeles cambiados. Los originales de tráileres anteriores se conservan.

Herramientas y comandos: [README de producción](../tools/trailer/README.md). Esta entrega no publica vídeo, ROM, assets HD ni una nueva release.
