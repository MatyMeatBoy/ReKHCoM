# Linux #330 — prueba de arranque publicada

El 28/10/2026 se publicó [v2.2-test330-startup](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.2-test330-startup), prerelease independiente dirigida a obtener un registro útil del arranque. Se [envió al reportero](https://github.com/MatyMeatBoy/ReKHCoM/issues/330#issuecomment-6479421077). Latest sigue en **v2.2**; los paquetes anteriores no se reemplazaron. No confirma resolver el fallo físico de Steam Deck/Gaming Mode ni demuestra que sus advertencias de overlay sean la causa.

## Identidad y fuentes

- Target/tag: `ef4091e1ddf32c8a970bac5fad0359156bb6fa48`, con master de películas452, diagnóstico457 y opción manual458 integrados. PR455/importación básica123 no está incluido; no se cierra todo #449.
- Archivo: `ReKHCoM-v2.2-test330-startup-linux-x86-x64-desktop-steamdeck.tar.gz`, **37 009 001 bytes**.
- SHA256: `a2e1ec91b98e2781904eadc233aef8875f52e45a76ba32281847a77e6fd76e56`.
- Los tres assets públicos (TAR, `SHA256SUMS.txt`, `BUILD-SOURCES.json`), sus hashes remotos, target del tag y estado public/prerelease se verificaron. La etiqueta interna del juego permanece v2.2; no identifica por sí sola esta prueba.

No fue una recompilación completa: se reutilizaron objetos del productor `07d5e1458942b7573cb8d855d0f0a3a4f6cdc661` tras comparar blobs relevantes y flags reales con el target. Se recompiló `port_hdmovie.o` en cada nativo32 y `port_hdmovie.o`/`sdl_shim.o` en cada guest64; el host64 es nuevo del target ef4091e1. El manifiesto registra las identidades empaquetadas (nativos32 tras el strip-debug habitual):

| Componente | SHA256 empaquetado |
| --- | --- |
| native32 EU | `58e7f270c757fadd5369b42837a1f55d2ea7bd4b7c330a967fb68d61187132ea` |
| native32 US | `66c7731d787ef7f5c92b044f8464bb78516ed69501cf8714ba46785a28019b04` |
| native32 JP | `f5754ecd3168d67dd418aed433ec50ebd08c70258b1c9e0a10116bef4052793f` |
| guest64 EU | `7bc1a04ee1537385b0cd2c98983a171fc1b5d83926dc3268887713ef5fbc4cfc` |
| guest64 US | `d8c7de08b364f5750b7eaaa3f8ac6ba032820d1734948a4b5b4663ccde085194` |
| guest64 JP | `a48b184a5daf2ff71f82364adc2372c5d08f2f236d744787c359454c3d26ea66` |
| host64 ef4091e1 | `f203141ed0a6fc85d00353d225caa896b9f309e777149666ede2312159d19f55` |

Los335 assets core coinciden con el catálogo actual. No se distribuyen ROM, SAV ni registros privados. Bibliotecas y assets conservan la base test439/441, TAR SHA256 `a0db3dec9d541388634ba152b5c4b0a9b5ced7a270589653b42f8e10448268a0`.

## Contrato y comprobación del TAR exacto

El arranque normal conserva selección/defaults. `--safe-startup` es una alternativa **manual, sólo para esa ejecución**: nativo32, bibliotecas incluidas y SDL software. No cambia automáticamente la GPU ni persiste esa elección. Los nuevos hitos flushed registran entrada/retorno/error de SDL, ventana, renderer y core; un core que sale por `H_exit` puede terminar correctamente sin emitir `guest-core returned`.

`FINAL-CONSUMER-GATE.json` pasó46 comprobaciones con el TAR exacto en una VM Debian sin red, CPU SSE4.1, SDL dummy/software, desde otro directorio de trabajo:

| Caso EU | Resultado observado |
| --- | --- |
| Auto/default64 | Título real, exit0, marcador boot1 |
| Force64 explícito | Título real, exit0, marcador boot1 |
| Safe32 con preferencia64 y versión antigua | Título real, exit0; preferencia y versión persistidas conservadas |
| Driver de vídeo inexistente | Error SDL-init64, host4/marcador0; fallback Auto32 existente, también rechazado por el mismo driver; exit final1 esperado |

Se verificaron rotación del log anterior y hashes idénticos de ROM/SAV privados copiados, `decompui.ini` y un INI ajeno. Los valores centinela del INI activo se conservaron, **sin afirmar igualdad de todos sus bytes**: Quit serializa normalmente `kh_com.ini`, cambia `assets_seen=4→0` y en64 añade el default ausente `mmpx=0`. Los deltas quedan registrados. Los intentos previos fallidos del observador/fixture se preservaron; no se clasificaron como fallos del producto.

Es QA funcional headless EU, sin medida de FPS, escucha física, GPU, Gamescope ni validación física Deck. Los componentes US/JP tienen build/provenance; no se les atribuye esta ejecución final EU. La confirmación externa de #330 sigue pendiente.

## Recibos conservados en el workspace

- `pruebas/issue330-entry-diagnostic/test330-delivery/`: `BUILD-RECEIPT.json`, `BUILD-SOURCES.json`, `PACKAGE-RECEIPT.json`, `FINAL-CONSUMER-GATE.json`, `ROOT-FINAL-AUDIT.json` y `SHA256SUMS.txt`.
- `releases/v2.2-test330-startup/PUBLICATION-RECEIPT.json`: publicación, readback remoto y respuesta del reportero.
- Raw final: `D:/01Claude/ReKHCoM-build/qa-runtime/test330-consumer-attempt3/raw/test330-proof/`, con cuatro casos, `RESULTS.json`, logs/rotación y hashes antes/después; `QEMU-OWNER.json` y consola en su directorio padre.
- TAR cerrado: `D:/01Claude/ReKHCoM-build/qa-runtime/test330-startup/candidate2/`.

La VM terminó y se devolvió el lease sin QEMU propio activo. Publicar esta prueba no cierra #330 ni altera el alcance de Android #374.
