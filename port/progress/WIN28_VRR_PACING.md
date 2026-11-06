# VRR Windows64 y contador de imágenes — preparación2.9

El guest Windows64 usa ABI Linux x32 y no define `_WIN32`. El selector y la
consulta de elegibilidad quedaban detrás de ese macro: sólo No/VSync y nunca
VRR. El host real comunica ahora dos capacidades opcionales por OUT1: control
VRR y elegibilidad de presentación DXGI. El guest consulta ambas; otros hosts
y versiones antiguas conservan su fallback. No cambian slots ni tablas ABI.

La consulta DXGI es la misma en nativo32/host64: swapchain D3D11/12, windowed y
ALLOW_TEARING. Esto no prueba FreeSync activo en monitor/driver. Si el intento
VRR falla y VSync funciona, la caché conserva la capacidad efectiva, de modo
que la consulta periódica pueda volver a intentar el camino recuperado.

El contador FPS incrementaba cada Pace, aunque FrameSkipDue omitiera dibujar
y presentar. Ahora incrementa después de SDL_RenderPresent aceptado. En el
bridge, aceptar la llamada no certifica scanout físico; sí excluye los ciclos
omitidos. La lógica conserva su contador propio y59,7275Hz. No se modifica el
temporizador, el zoom, el salto de cuadros ni la calidad para disfrazar carga.

Validación: fixtures C reales native/guest VRR/fallback/reintento y38/28
contratos de output PASS; selector guest interpretado y reintento RED/GREEN;
Windows64 EU compilado con jobs1, pruebas ABI/mapping/boot/BIOS PASS. La sonda
local D3D11 oculta1440p reproduce campo x2: canvas1280x720, PPU18,6ms en una
ventana de60 ciclos,19omitidos y~41presentaciones/s con lógica~59,7. Campo x4
reduce carga. No es el PC6900XT del usuario ni validación VRR física.

La fluidez completa sigue abierta: reducir coste PPU, contrastar antes/después
con mismos píxeles, contar omisiones y medir distribución de intervalos por
modo/plataforma antes de publicar2.9. Evidencia privada:
`pruebas/pacing-v28-6900xt-20261106/`.
