# Metadatos opcionales del mando real en hosts de 64 bits

Claim `controller-bridge-metadata`, owner `codex-control-prompts`.
Rama `feature/controller-bridge-metadata`, independiente de los PR de prompts y GPU.

Windows64, Linux64 y Android64 atienden la consulta reservada
`KHCOM_GAMEPAD_INFO_V1` mediante el `getenv` ya existente. La respuesta exacta
`bridge-pad-v1:1:<connected>:<type>` identifica el único mando físico que esos
hosts publican en el slot de puente 1. No se transmite el joystick ID de SDL.
`connected` viene de `SDL_GamepadConnected`; la familia viene del tipo real de
SDL, mediante valores explícitos 0..10 (desconocido, estándar, Xbox360/One,
PS3/4/5, Switch Pro/JoyCon izquierdo/derecho/par). Un tipo futuro queda desconocido.
No cambia `HostPad`, `HF_COUNT`, la tabla ni los opcodes del puente.

El shim consulta una vez por Refresh de eventos, con 48 bytes de pila y sin
asignaciones. Los getters reutilizan la copia; no hay consulta por glifo/draw.
El estado físico separa el mando real del teclado emulado de hosts antiguos.
Los hosts nuevos sin mando dan NULL en FromID y una lista vacía: los prompts
pueden usar teclado. La entrada pad/teclado del puente conserva sus eventos.
La normalización Nintendo existente consume el tipo de la fuente física;
seleccionar una familia visual no cambia estos metadatos ni los bindings.

Hosts antiguos y consolas sin la extensión conservan el camino original,
STANDARD/genérico. Respuestas truncadas, versión/ID incorrecto, números fuera
de rango y texto sobrante se rechazan con ese mismo fallback; nunca se adivina
PlayStation por el nombre ni por la presentación elegida. En esos hosts no se
promete detectar el mando físico detrás de su teclado emulado.

## Evidencia y límites

`python port/tools/test_bridge_gamepad_metadata.py` compila el productor real
compartido, los tres H_getenv reales y las funciones reales Refresh/SDL_GetGamepads/
FromID/Connected/Type del shim. RED anterior: PS5 informado devuelve STANDARD.
GREEN: todas las familias, tipo futuro desconocido, ausencia/desconexión/reconexión,
ID de puente, fallback antiguo, metadatos malformados y buffers insuficientes.
Mil getters por tipo no añaden consultas. Se usan asserts del CRT nativo, no el
assert.h vacío del guest. El test no requiere ROM, GPU ni un mando conectado.

Falta QA física de Xbox/Nintendo/PlayStation y hotplug en Windows/Linux/Android
con los prompts experimentales; este PR no asegura equivalencia visual completa.
Original→activar durante página ya abierta y fallback textual variable siguen
pendientes en los PR de prompts; esta extensión no los cierra. Sin build pesado,
sin acceso a la instalación protegida ni cambios de partidas.
