# Primera medición OptiPort — 27/10/2026

Build EU `249c55e887b56b09afa36e36452baab07b8a5079`, Windows, Direct3D11, ventana oculta 1920x1080, audio dummy, un worker PPU, sin frame skip ni fast-forward. ROM y datos de prueba aislados; instalación principal intacta. Vídeo y música HD apagados para aislar gráficos. HUD/texto/cartas/título se alternan juntos. Import de sprites existente, con algunos recursos rechazados que conservan el original; no es una certificación de import completo.

Arnés `port/tools/profile_hd_resources.py`, 600 cuadros por caso, primeros 240 separados como cold. Datos completos locales en `build/hd-resource-profile/20261027-022208-480769/report.json`, con hash del ejecutable y logs. Valores de memoria son RSS del proceso, **no VRAM**.

| Escenario | Original: trabajo medio / p99 | HD: trabajo medio / p99 | RSS pico original / HD |
| --- | --- | --- | --- |
| Título | 9,33 / 60,17 ms | 5,03 / 10,07 ms | 327,50 / 380,88 MiB |
| Monstruo combate 1 | 5,44 / 9,71 ms | 6,01 / 20,87 ms | 329,54 / 341,07 MiB |
| Monstruo combate 2 | 5,07 / 10,22 ms | 6,05 / 22,23 ms | 329,85 / 340,43 MiB |
| Puerta de evento | 9,00 / 13,37 ms | 11,91 / 33,09 ms | 329,91 / 347,91 MiB |

La primera muestra de título tiene ruido evidente; **no concluir que HD sea más rápido**. Son observaciones iniciales de una corrida, no una mejora porcentual ni evidencia de OOM. Repetir muestras tras las correcciones y separar IO/primer upload de trabajo estable antes de optimizar por estos números.

El selector de puerta de esta corrida utiliza la fixture de presión y sólo se mide hasta cuadro 600; no sustituye la regresión funcional de entrada completa a 3000 cuadros documentada en DOOR_PORTABILITY_277.md. Los casos de Monstruo alcanzan `mode_battle` en cuadro 242.

Siguientes mediciones: presupuesto de targets de gauges a altas resoluciones, comparación GPU tras cambiarlo, reproducción real de música/vídeo y transiciones prolongadas. El arnés evita pulsar A en título: abrir el submenú modal invalidaba el límite de cuadros y la corrida, por lo que esas ejecuciones fallidas no forman parte de esta tabla.
