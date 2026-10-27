# Retorno de galería: evidencia dirigida — 27/10/2026

El pendiente de evidencia de #339 queda verificado en el alcance EU x86
descrito aquí: once casos con framebuffer D3D11 oculto, fuente productiva
`1c0b6c7e4166b2a25364cda1b2ac9a776989b4db`. No se modificó código productivo,
la instalación del jugador, el launcher ni los artefactos publicados.

## Resultado y alcance

Título/partida × GBA/ReKHCoM × fin normal/skip: **8/8**. Controles HD puro
normal, ReKHCoM ausente y ReKHCoM corrupto: **3/3**. En todos, el primer cuadro
de retorno y los ocho siguientes tienen canvas de galería activo y fade cero.
Se inspeccionaron los cuarenta cuadros consecutivos retenidos alrededor de
cada retorno: la escena pasa a la lista sin presentar título/campo restaurado.
El oscurecimiento final propio de la escena sigue presente donde corresponde.

Los once casos conservan SHA256 de SRAM, INI serializado, escenas vistas y
archivos de progreso. La auditoría productiva exige
`game=1 map=1 config=1 files=1 recom=1 rank=1`; el recurso corrupto produce dos
auditorías válidas, una del intento HD y otra del fallback nativo. La selección,
evento, pestaña, personaje y scroll actual permanecen iguales al volver;
esta fixture utiliza Sora/evento10 y scroll0. El test de resume independiente
también comprueba scroll2 y memorias de personaje/pestaña.

El skip se verifica con pausa y página de confirmación realmente presentadas.
El vídeo HD interceptado aparece en dos sitios distintos de `port_hdmovie.c`:
1178 (vídeo) y565 (pausa). Esto es evidencia de ejecución del hook, no una
inferencia a partir de su declaración. El contador de presentaciones es propio
del recorder y no depende de que el reloj de la lógica avance durante HD.

**Límites:** lectura del backbuffer antes de cada `SDL_RenderPresent`, ventana
oculta, SDL audio dummy; no es una medición de rendimiento, presentación física,
audio, foco/reset, Windows64, Linux/Deck, Android ni consolas. No acredita
duraciones ni toda la historia. El core cacheado `f347461f…`, de revisión fuente
desconocida, no se reutilizó ni se atribuyó a HEAD.

## Harness y procedencia

`gallery_return_qa.py` configura una única build EU i686 fresca y solicita
explícitamente `khcom.exe` y `SDL3.dll`; no ejecuta el target por defecto que
también construiría el launcher. El Ninja de QA aplica forced-include a todas
las unidades productivas con `SDL_RenderPresent` y enlaza el recorder. El
wrapper `gallery_ui_probe.c` incluye `port_ui.c` intacto para observar sus
estáticos; no cambia sus decisiones. Ningún archivo del harness entra en
builds ordinarias.

El recorder mantiene un ring de32 superficies: al detectar retorno guarda
31 cuadros anteriores, el primer retorno y8 posteriores, más una referencia
de lista anterior al replay. Los BMP temporales se convierten sin pérdida en
PNG; el CSV conserva todos los sitios de presentación y sus estados. Hay
41 imágenes por caso y una contact sheet que muestra todas las retenidas.
Los recursos de UI están embebidos en el core; ROM, save e imports son copias
privadas y no se incluyen en este PR. La fixture HD es un testsrc2 sintético
de2 segundos, generado localmente con ffmpeg.

El manifiesto y los once resultados saneados están en
[GALLERY_RETURN_QA_V22.json](GALLERY_RETURN_QA_V22.json). Ejecutable de QA:
`738a236c1e2b30b0ff54231dcd369e95b93d3c15a5cba9ed8646a2a4707f1528`.
El árbol privado `pruebas/v22-gallery-return` conserva `build-manifest.json`,
`results.json`, el resumen visual `before-after.png` y, por caso, `run.log`,
`qa-frames.csv`, PNG originales y `contact.png`. Los hashes de CSV/contact
sheet están en el JSON versionado. La raíz puede revisar las imágenes sin
copiar ROM, partidas ni bibliotecas importadas al repositorio.

## Reproducción acotada

Usar Python con Pillow/PyYAML, ffmpeg y toolchain/SDL3 i686 existentes.
`--config` debe ser un INI completo previamente serializado por esta build
en una copia aislada. El primer probe detectó correctamente que un INI mínimo
no es byte-idéntico al INI completo escrito por `PortConfigSave` al salir;
se normalizó antes de medir preservación. La auditoría de configuración del
replay también se exige en memoria, independientemente del archivo.

```powershell
python port/tools/gallery_return_qa.py --build --work RUTA_PRIVADA --rom ROM_EU --save SRAM_INICIALIZADA --config INI_SERIALIZADO --toolchain LLVM_MINGW --sdl SDL3 --ninja NINJA
python port/tools/gallery_return_qa.py --all --work RUTA_PRIVADA --rom ROM_EU --save SRAM_INICIALIZADA --config INI_SERIALIZADO
python port/tools/test_gallery_direct_return.py
```

Primero se ejecuta ReKHCoM título/skip para demostrar captura de vídeo/pausa;
`--all` reutiliza ese resultado solo con binario, SRAM, INI e imágenes idénticos.
Los casos se ejecutan serialmente, con `KHCOM_HIDDEN=1`, driver D3D11 real y
FAST ausente. `KHCOM_GALLERY_AUTO_KEYS=skip` por sí solo no acredita el menú
de pausa: inyecta bits GBA. Para skip nativo se usa el harness existente
`KHCOM_OPTIONS_TOGGLE=240` y `KHCOM_GALLERY_NATIVE_PAUSE=skip`, que recorren
el menú productivo de pausa/confirmación. No se aceptó como PASS ningún intento
en el que la página de confirmación faltaba; esos probes se conservaron aparte.

El helper previo de resume quedó desactualizado tras incorporar el estado
Sora/Riku. Se reprodujo su fallo de compilación, se actualizaron únicamente sus
stubs y aserciones de preservación, y el test volvió a pasar. No se sustituyó
la evidencia runtime por el resultado de ese helper.

## Rebase documental

El PR se rebasó sobre `e20a640e8` para conservar el tablero vigente, incluidas
las filas separadas de #330 y Moto Android. Los archivos productivos de galería,
UI, presentación y eventos no cambiaron entre la fuente probada y esa base; no
se reconstruyó ni se atribuyó el binario de QA a un SHA nuevo. La fuente probada
sigue siendo `1c0b6c7e4` y el ejecutable conserva el hash registrado arriba.
