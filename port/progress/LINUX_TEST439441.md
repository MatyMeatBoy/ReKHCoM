# Linux: test dirigido de vídeo HD y título

Fuentes de juego, host y empaquetado: `07d5e1458942b7573cb8d855d0f0a3a4f6cdc661`.
Incluye Linux32 y Linux64 con EU/US/JP. La estable v2.2 y sus archivos publicados permanecen inmutables.

## Cambios y alcance

- #439 / PR447: el lector de vídeo Linux64 sustituye la espera fija tras una lectura vacía por espera de disponibilidad del pipe con timeout acotado. Conserva entrada, formato, cadencia, reloj de audio y calidad. No modifica archivos importados.
- #441 / PR446: el paquete Linux incluye el catálogo común completo de 335 recursos. Antes faltaban 135, incluidos 126 del título. El A/B del mismo ejecutable con sólo las cinco piezas inglesas de Press Start restaura la T.
- PR448: la barra HD conserva la composición nativa cuando alguna parte activa del texto HD no fue realmente aceptada por la cola. La existencia del archivo no sustituye esa comprobación.
- Los guests se recompilaron de la fuente indicada, incluidos los cambios posteriores a estable434/436. No atribuir estos bytes al tag estable original.

## Evidencia previa y del paquete

El cambio de439 conserva60 cuadros distintos y sus hashes muestreados en orden en tres comparaciones nativas emparejadas con el mismo H264/AAC1080p30. El intervalo mediano entre retornos de presentación pasa de aproximadamente110–132ms a33,13–33,29ms. Son pruebas locales con renderer software; no prueban rendimiento físico de Steam Deck ni un hash de todos los píxeles decodificados.

El título tiene nueve controles Linux de empaquetado/idiomas/modo original, doce controles nativos Linux32/64 de recursos válidos/ausentes/corruptos y65 cuadros Windows sin cambios en los positivos. La corrección del caso negativo restaura la máscara nativa de la T; los cuadros HD válidos no necesitan coincidir con la fuente GBA.

Archivo cerrado: `ReKHCoM-v2.2-test439-441-linux-x86-x64-desktop-steamdeck.tar.gz`,37004056bytes, SHA256 `a0db3dec9d541388634ba152b5c4b0a9b5ced7a270589653b42f8e10448268a0`.
Todos sus miembros se contrastaron con el staging y con el catálogo común. No incluye ROM, partidas, logs privados, configuración personal ni vídeos importados.

**Entrega pública:** [v2.2-test439-441](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.2-test439-441), prerelease con tres assets y digests remotos verificados. El tag apunta a la fuente indicada; Latest sigue siendo v2.2.

El consumidor del TAR exacto pasa siete controles: seis títulos dirigidos mediante el launcher real, EU/US/JP en32/64, y una apertura HD mediante el modo de película real del juego. EU/US aceptan el texto HD; JP usa su texto nativo porque las recetas inglesas sólo se compilan en EU/US. No se certifica un título japonés completamente HD ni un recorrido natural de todos los logos. La apertura reproduce los60 cuadros del mismo archivo H264/AAC1080p30, produce PCM no nulo y vuelve al modo Event. La prueba funcional usa FAST y renderer software: no sustituye la medición de cadencia previa ni confirma Gaming Mode o hardware físico.

Se conservan los intentos que detectaron errores del observador. El análisis final reúne tres casos correctos de cada uno de los intentos4/5 y la película del intento6. Esta última ya había reproducido60 cuadros, pero el primer parser omitía el log del host `ReKHCoM.log` y sólo leía `kh_com.log`; el reanálisis independiente conserva los resultados originales fallidos. La hipótesis de permisos de FFmpeg se descartó con su preflight previo al cambio de permisos. La VM queda apagada y sin procesos propios pendientes.

El publicador utiliza comprobaciones explícitas que siguen activas con Python-O. Nueve negativos y un positivo idempotente se validaron sin llamadas reales a GitHub ni red. Las verificaciones remotas y respuestas reales se ejecutaron después, con el gate final y el archivo cerrado.

Respuestas con enlace y solicitud de comparación física: [439](https://github.com/MatyMeatBoy/ReKHCoM/issues/439#issuecomment-6476426150) y [441](https://github.com/MatyMeatBoy/ReKHCoM/issues/441#issuecomment-6476427558). Ambos siguen abiertos para confirmar el resultado en el equipo del reportero. El nuevo449 sobre volumen de películas se declara fuera de este test y se atiende en la continuación de OptiPort.

Evidencia privada: `pruebas/issue439-linux-video-pacing/` (`CONSUMER-GATE.json`, `ROOT-DELIVERY-QA.json`, `PUBLISHER-OPTIMIZED-PRESSURE.json`) y `pruebas/issue441-title-start-bar/`. Fuentes, sumas y límites acompañan al test público; la confirmación del reportero queda separada de las regresiones locales.

## Continuación

Primero publicar y enviar el test439/441; después completar los criterios de la [pasada de OptiPort](OPTIPORT_FULL_PASS_20261028.md), y después los demás pendientes. La reducción a720p queda sólo como boceto interno aplazado por el usuario: no forma parte de esta entrega ni de las acciones automáticas posteriores.
