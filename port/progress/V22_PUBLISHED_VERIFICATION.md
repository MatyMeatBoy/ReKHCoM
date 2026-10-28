# v2.2 publicada — 28/10/2026

[Release estable y paquetes](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.2),
marcada como Latest. Tag: `2ab2ebb6b739a12ae864dc1fa66dbd9f9638b4fc`.
Los ocho paquetes publicados coinciden en tamaño y SHA256 con sus candidatos
cerrados. Quiver selecciona exactamente un paquete para Windows, Linux x64 y
Android. Fuentes, hashes y límites están en los adjuntos `BUILD-SOURCES-v2.2.json`
y `SHA256SUMS-v2.2.txt`; no se sustituyeron v2.1 ni los prereleases.

| Paquete | Fuente de compilación o empaquetado | Alcance |
| --- | --- | --- |
| Windows x86/x64 | `d43f6ab2315d6f461396c64ef8da67bb7a1bccc3`; juego `38f84bd36b176c926be2d236fdf320eb844c152b` | Launcher actualizado; binarios del juego idénticos a los ya probados. |
| Linux desktop/Steam Deck x86/x64 | `d43f6ab2315d6f461396c64ef8da67bb7a1bccc3`; juego `ace4116e97e54eff34ca4cb2035ea46d2be1f261` | Scripts/version actualizados; binarios idénticos. Gaming Mode del reportero sigue pendiente. |
| Anbernic ARMhf beta | Las mismas fuentes de empaquetado/juego de Linux | Tres regiones y paquete verificados; dispositivo físico sin verificar. |
| Android ARM32/ARM64 beta | `38f84bd36b176c926be2d236fdf320eb844c152b` | Tres regiones, identidad, firma y archivo verificados; Moto/S22 siguen abiertos. |
| PS4 beta | `ace4116e97e54eff34ca4cb2035ea46d2be1f261` | SDK, paquete y SFO verificados; consola física sin verificar. |
| PS5, Vita y Switch beta | `5ccb8d24868ec9fb0a286803f84ad4bf2b3f840c` | SDK, regiones y paquetes/metadatos verificados; consolas físicas sin verificar. |

## Web

[Web beta](https://matymeatboy.github.io/ReKHCoM/), fuente
`5ccb8d24868ec9fb0a286803f84ad4bf2b3f840c`, publicación Pages
`2c07b788d363fe49e59c90279c2e2f244c26f4ab`. Los treinta archivos remotos
coinciden con el sitio congelado. Las tres regiones compilaron; auditoría Wasm,
ABI, controles, editor de partidas e interfaz inicial pasaron.

El arranque con ROM, el regreso del mapa y las transiciones de pantalla del
juego en navegador siguen sin verificar. La extensión bloqueó el selector de
archivo y la automatización nativa se detuvo al no poder confirmar la URL.
No se cambiaron permisos ni se convirtió este bloqueo de herramienta en un
fallo del producto o un resultado de juego aprobado.

## Regresiones y entrega a reporteros

- Windows: seis arranques regionales/arquitecturas, mapa con copia de partida
  en 32/64 y regreso, 21 contratos DIV, capturas HD y señales PCM dirigidas.
  El launcher reempaquetado pasó cinco casos de migración/persistencia.
- Linux: seis arranques, mapa/retorno 32/64, importador/MP4 y 21 contratos DIV;
  doce casos con FFmpeg real cubren pausa, salto, FPS, resize y quit/reset.
  El script del TAR final pasó tres casos de migración. No son pruebas físicas
  de Steam Deck ni mediciones de rendimiento general.
- Nueve respuestas publicadas con enlaces reales a v2.2. Se cerraron
  [#418](https://github.com/MatyMeatBoy/ReKHCoM/issues/418#issuecomment-6467226434)
  y [#421](https://github.com/MatyMeatBoy/ReKHCoM/issues/421#issuecomment-6467228284)
  por las correcciones reproducidas; esto no equivale a confirmación del equipo
  del reportero. [#407](https://github.com/MatyMeatBoy/ReKHCoM/issues/407#issuecomment-6467227669)
  conserva su nuevo comentario y queda abierto para probar el fix publicado.
- Siguen abiertos #407, #412, #374, #330, #336, #378 y #163 con seguimiento
  específico. #360 y #380 ya tenían confirmación previa; no reabrirlos usando
  una lista histórica ni atribuirles causas que no fueron demostradas.

La revisión nueva #432 de organización de barajas continúa aparte de esta
entrega. Los recortes de Jaula parásita/fondos de subida de nivel fueron
aplazados expresamente por el usuario y conservan sus tareas en la cola.
Publicar 2.2 no completa toda la cola ni demuestra ausencia universal de bugs.
