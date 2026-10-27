# Retorno de galerÃ­a: evidencia dirigida â€” 27/10/2026

El pendiente de evidencia de #339 queda verificado en el alcance EU x86
descrito aquÃ­: once casos con framebuffer D3D11 oculto, fuente productiva
`1c0b6c7e4166b2a25364cda1b2ac9a776989b4db`. No se modificÃ³ cÃ³digo productivo,
la instalaciÃ³n del jugador, el launcher ni los artefactos publicados.

## Resultado y alcance

TÃ­tulo/partida Ã— GBA/ReKHCoM Ã— fin normal/skip: **8/8**. Controles HD puro
normal, ReKHCoM ausente y ReKHCoM corrupto: **3/3**. En todos, el primer cuadro
de retorno y los ocho siguientes tienen canvas de galerÃ­a activo y fade cero.
Se inspeccionaron los cuarenta cuadros consecutivos retenidos alrededor de
cada retorno: la escena pasa a la lista sin presentar tÃ­tulo/campo restaurado.
El oscurecimiento final propio de la escena sigue presente donde corresponde.

Los once casos conservan SHA256 de SRAM, INI serializado, escenas vistas y
archivos de progreso. La auditorÃ­a productiva exige
`game=1 map=1 config=1 files=1 recom=1 rank=1`; el recurso corrupto produce dos
auditorÃ­as vÃ¡lidas, una del intento HD y otra del fallback nativo. La selecciÃ³n,
evento, pestaÃ±a, personaje y scroll actual permanecen iguales al volver;
esta fixture utiliza Sora/evento10 y scroll0. El test de resume independiente
tambiÃ©n comprueba scroll2 y memorias de personaje/pestaÃ±a.

El skip se verifica con pausa y pÃ¡gina de confirmaciÃ³n realmente presentadas.
El vÃ­deo HD interceptado aparece en dos sitios distintos de `port_hdmovie.c`:
1178 (vÃ­deo) y565 (pausa). Esto es evidencia de ejecuciÃ³n del hook, no una
inferencia a partir de su declaraciÃ³n. El contador de presentaciones es propio
del recorder y no depende de que el reloj de la lÃ³gica avance durante HD.

**LÃ­mites:** lectura del backbuffer antes de cada `SDL_RenderPresent`, ventana
oculta, SDL audio dummy; no es una mediciÃ³n de rendimiento, presentaciÃ³n fÃ­sica,
audio, foco/reset, Windows64, Linux/Deck, Android ni consolas. No acredita
duraciones ni toda la historia. El core cacheado `f347461fâ€¦`, de revisiÃ³n fuente
desconocida, no se reutilizÃ³ ni se atribuyÃ³ a HEAD.

## Harness y procedencia

`gallery_return_qa.py` configura una Ãºnica build EU i686 fresca y solicita
explÃ­citamente `khcom.exe` y `SDL3.dll`; no ejecuta el target por defecto que
tambiÃ©n construirÃ­a el launcher. El Ninja de QA aplica forced-include a todas
las unidades productivas con `SDL_RenderPresent` y enlaza el recorder. El
wrapper `gallery_ui_probe.c` incluye `port_ui.c` intacto para observar sus
estÃ¡ticos; no cambia sus decisiones. NingÃºn archivo del harness entra en
builds ordinarias.

El recorder mantiene un ring de32 superficies: al detectar retorno guarda
31 cuadros anteriores, el primer retorno y8 posteriores, mÃ¡s una referencia
de lista anterior al replay. Los BMP temporales se convierten sin pÃ©rdida en
PNG; el CSV conserva todos los sitios de presentaciÃ³n y sus estados. Hay
41 imÃ¡genes por caso y una contact sheet que muestra todas las retenidas.
Los recursos de UI estÃ¡n embebidos en el core; ROM, save e imports son copias
privadas y no se incluyen en este PR. La fixture HD es un testsrc2 sintÃ©tico
de2 segundos, generado localmente con ffmpeg.

El manifiesto y los once resultados saneados estÃ¡n en
[GALLERY_RETURN_QA_V22.json](GALLERY_RETURN_QA_V22.json). Ejecutable de QA:
`738a236c1e2b30b0ff54231dcd369e95b93d3c15a5cba9ed8646a2a4707f1528`.
El Ã¡rbol privado `pruebas/v22-gallery-return` conserva `build-manifest.json`,
`results.json`, el resumen visual `before-after.png` y, por caso, `run.log`,
`qa-frames.csv`, PNG originales y `contact.png`. Los hashes de CSV/contact
sheet estÃ¡n en el JSON versionado. La raÃ­z puede revisar las imÃ¡genes sin
copiar ROM, partidas ni bibliotecas importadas al repositorio.

## ReproducciÃ³n acotada

Usar Python con Pillow/PyYAML, ffmpeg y toolchain/SDL3 i686 existentes.
`--config` debe ser un INI completo previamente serializado por esta build
en una copia aislada. El primer probe detectÃ³ correctamente que un INI mÃ­nimo
no es byte-idÃ©ntico al INI completo escrito por `PortConfigSave` al salir;
se normalizÃ³ antes de medir preservaciÃ³n. La auditorÃ­a de configuraciÃ³n del
replay tambiÃ©n se exige en memoria, independientemente del archivo.

```powershell
python port/tools/gallery_return_qa.py --build --work RUTA_PRIVADA --rom ROM_EU --save SRAM_INICIALIZADA --config INI_SERIALIZADO --toolchain LLVM_MINGW --sdl SDL3 --ninja NINJA
python port/tools/gallery_return_qa.py --all --work RUTA_PRIVADA --rom ROM_EU --save SRAM_INICIALIZADA --config INI_SERIALIZADO
python port/tools/test_gallery_direct_return.py
```

Primero se ejecuta ReKHCoM tÃ­tulo/skip para demostrar captura de vÃ­deo/pausa;
`--all` reutiliza ese resultado solo con binario, SRAM, INI e imÃ¡genes idÃ©nticos.
Los casos se ejecutan serialmente, con `KHCOM_HIDDEN=1`, driver D3D11 real y
FAST ausente. `KHCOM_GALLERY_AUTO_KEYS=skip` por sÃ­ solo no acredita el menÃº
de pausa: inyecta bits GBA. Para skip nativo se usa el harness existente
`KHCOM_OPTIONS_TOGGLE=240` y `KHCOM_GALLERY_NATIVE_PAUSE=skip`, que recorren
el menÃº productivo de pausa/confirmaciÃ³n. No se aceptÃ³ como PASS ningÃºn intento
en el que la pÃ¡gina de confirmaciÃ³n faltaba; esos probes se conservaron aparte.

El helper previo de resume quedÃ³ desactualizado tras incorporar el estado
Sora/Riku. Se reprodujo su fallo de compilaciÃ³n, se actualizaron Ãºnicamente sus
stubs y aserciones de preservaciÃ³n, y el test volviÃ³ a pasar. No se sustituyÃ³
la evidencia runtime por el resultado de ese helper.

## Rebase documental

El PR se rebasó sobre `e20a640e8` para conservar el tablero vigente, incluidas
las filas separadas de #330 y Moto Android. Los archivos productivos de galería,
UI, presentación y eventos no cambiaron entre la fuente probada y esa base; no
se reconstruyó ni se atribuyó el binario de QA a un SHA nuevo. La fuente probada
sigue siendo `1c0b6c7e4` y el ejecutable conserva el hash registrado arriba.
