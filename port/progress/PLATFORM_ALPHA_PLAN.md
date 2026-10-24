## v1.9 PUBLICADA — 17/10/2026

- Release publica estable Windows: https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v1.9
- Android Alpha3 ARMv7 actualizado adjunto a la misma release. Build EU/US/JP,
  parser48 y touch PASS; NO se comprobo en ARM fisico. Mantener etiqueta alpha.
- Web Alpha3: motores EU/US/JP nuevos; Wasm validado/compilado en Node, JS sin
  errores de sintaxis. Pages desplegado; verificacion HTTP/SHA en reporte adjunto.
  https://matymeatboy.github.io/ReKHCoM/
- Windows: PPU206, Riku7, launcher11/import6, fondos circular/rectangular, input
  opt-in/hold/foco/overlay y guardado aislado PASS. Paquetes sin ROM/save/config.
- Devolver acumuladas: runtime OFF inerte; ON devuelve1 y3 cartas asentadas y
  recupera mano21. Fixture3: KHCOM_BATTLE=0:1:0, modo200:hudhp:100, STOCK300/450/600,
  UNSTOCK760-800. Intento con300/360/420 solo acumulo2 por combate/animacion:
  no asumir que cada input del harness logra acumular una carta.
- Instalacion local del BAT actualizada; hashes de saves, INI y estados intactos.
  Copia del savestate anterior + EXE exacto guardada PRIVADA para diagnostico.
- Git main y tagv1.9 actualizados al cierre; snapshot previo preservado como tag
  v1.9-preview-20261017. Nunca subir fixtures privados de build/ ni arte extraido.
- Linux/Steam Deck/RG34XX siguen SIN binario nativo: ABI32/64 pendiente.
  Otros cheats/editor, menus relativos experimentales y zoomx3 reportado pendientes.
  No anunciar esas tareas como terminadas ni compatibilidad de dispositivos no probados.

## Cierre v1.9 autorizado (17/10/2026)

- Usuario autoriza publicar v1.9 tras corregir los bordes de Riku; sustituye
  la restriccion anterior de mantenerla privada. Windows estable; Android/Web alpha.
- Audio: usuario confirma que suena; prueba WASAPI con mezcla no silenciosa.
- Cargar: Riku SOLO con truco ON o Sora completado. 7 pruebas reales pasan,
  incluidos checksum de cabecera de fixture completado, OFF oculta sin borrar,
  ON carga/guarda Riku con las otras ranuras intactas. Original privado intacto.
- Devolver cartas acumuladas vuelve SOLO en Experimental, default OFF.
  Nueva clave experimental_unstock; ignora unstock_button antiguo y bit512 de
  metadatos. No incluido en paquete QoL de nueva partida. Mantener V / stick
  derecho pulsado o hacia abajo 0.5s, solo Sora; no cancela tecnica en curso.
  Procedencia: ggdrn/KH_COM_VITA (MIT), no una mecanica GBA/PS2 nativa.
- Riku: primer circulo corregido + base rectangular opaca de cinematicas:
  extiende bandas de tiles nativos; rechaza tiles parcialmente transparentes,
  menus, batalla y foreground. Original 240x160 exacto. Pruebas x1/x4, fade,
  paleta y rechazo de geometria PASS. Savestate usuario respaldado con su EXE
  exacto en build/riku-hollow-user-review (PRIVADO, no subir).
- Linux/Steam Deck/RG34XX siguen BLOQUEADOS por ABI punteros32/64; no hay
  binario nativo probado. No etiquetar ARMv7 como ARM64 ni inventar soporte.
- Menus relativos experimentales excluidos. Otros cheats/editor siguen pendientes.

# Plataforma y reparto — 17/10/2026

## Regla del usuario

Actualizacion del usuario 17/10/2026: v1.9 es candidata; v1.9.1 sera la primera
publica estable con TODAS las plataformas. Sustituye la regla anterior no-PC alpha.
Solo estables se muestran como releases publicas. v1.9/web-alpha2 ocultas como
borradores privados; tags conservados. No publicar candidatas visibles.
Las compilaciones alpha siguen siendo alpha hasta completar su validacion;
no afirmar compatibilidad ni rendimiento de dispositivos no probados.
La nueva escala relativa de menus no entra en stable. No subir ROMs, saves ni
estados a Git. Conservar historial y trabajo de Claude; no integrar a ciegas.

## En curso: Android, luego Linux / Steam Deck

- Rama `feature/android-save-import`: launcher con destino regional y banco
  explicito (1..5), importacion/exportacion del banco correcto, guardado de la
  seleccion al recrear Activity y bloqueo de selectores simultaneos.
- `.sav` y `.srm` se interpretan por contenido, sin filtro de extension del picker
  SAF. Acepta SRAM de 32 KiB (relleno FF), 64 KiB y 128 KiB solamente cuando las
  dos mitades de 64 KiB son identicas. No recorta contenedores desconocidos.
- Verifica firma, checksum de al menos una cabecera y un bloque de datos.
  Conserva copias redundantes tal cual para la reparacion nativa del juego.
  EU usa VER13; US/JP VER12. US y JP no pueden distinguirse por esta firma:
  el jugador elige la region correcta. No es conversion entre regiones.
- Escritura temporal + sync, copia previa del archivo destino y rename.
  Exporta SRAM de 64 KiB compatible con el port. No importa savestates.
- `python port/tools/test_android_save_import.py` prueba el parser Java real,
  datos sinteticos, corrupcion, version regional y bancos. Se pueden pasar
  rutas locales adicionales para verificar identidad de partidas privadas.
- APK siguiente: `1.9-android-alpha2`, versionCode 19002. Compilar no demuestra
  funcionamiento en ARM fisico. x86 de emulador y ARMv7 son objetivos distintos.

## Pendientes que no deben perderse

1. Android: repetir ejecucion EU/US/JP con APK nueva; probar picker SAF real,
   restauracion de Activity, import/export y multitouch. HD y quickstates siguen
   desactivados. ARM64 requiere adaptar punteros de 32 bits; no esta resuelto.
2. Cheats: submenu y desbloqueo de Riku implementados y probados en PC;
   persistencia y carga/guardado de partidas previas corregidos (RIKU_BUGS_V19.md).
   Todo OFF por defecto; Riku disponible sin falsear la bandera
   de Sora completado. Cheats conocidos implementados por estructuras/funciones
   del juego, no codigos de direcciones de otra region. Editor de cartas con
   region + banco + ranura explicitos, checksum nativo, backup y limites.
   **Resto de cheats y editor de cartas pendientes.**
3. Linux: backend ELF32, SDL3, memoria GBA y fuentes portables. Steam Deck es
   objetivo especifico: SteamOS, mando, 1280x800, modo juego, selector de archivos
   utilizable y persistencia de saves. No anunciar Deck Verified sin certificacion.
   **Sin build Linux ni prueba Deck por ahora.**
   Objetivo confirmado por usuario: Anbernic RG34XX, H700 Cortex-A53, 1 GiB,
   Linux ARM64, 720x480 3:2. Mantener paquete distinto de Steam Deck x86-64.
   Firmware no concretado: estudiar PortMaster en muOS/ROCKNIX, no afirmar
   soporte para stock sin prueba. Referencias oficiales:
   https://anbernic.com/products/rg34xx y https://rocknix.org/devices/anbernic/rg34xx/.
4. 4:3 con todos los zooms y texto HD, sin deformar. Conservar tambien el modo
   original GBA 3:2. El usuario pidio conservar forma original; 4:3 es otra opcion.
   Rama `feature/aspect-43-zoom`: opciones 4:3 y GBA 3:2 implementadas,
   geometria 56 combinaciones PASS (incluye 720x480) y build EU Windows correcto. Regresion visual
   en todos los zooms y texto HD aun pendiente. No integrado en main/stable.
5. Web: rama `fix/web-mobile-files-controls`, commit 7956d77f6, subida pero NO
   integrada ni desplegada. Contiene correcciones de teclado/foco/fullscreen,
   import/export y compartir a Archivos. JS y C pasan tests; teclado real de
   Chrome comprobado con fixture sin ROM. Touch real en navegador y dispositivos
   iOS/Android sigue pendiente. Root Pages sigue Web Alpha 2.
6. Menus: rama `feature/relative-menu-scale`, geometria preliminar aun sin integrar
   al render. No cortar framebuffer por tercios: puede partir guantes, badges y
   texto. Anclar grupos completos y fondos propios; probar entrada/salida.
7. Rendimiento x1: baseline `build/zoom-benchmark/relative-menu-baseline/report.json`.
   Field PPU 28.34ms, battle 19.22ms (SDL dummy/software; no FPS de GPU real).
   Optimizar con comparacion de pixels y regresion PPU, sin cache invalida entre
   HBlank. No hay nueva optimizacion x1 en esta rama.

## Licencias y herramientas

Usuario autorizo instalar SDK/NDK y acepto licencias necesarias. AVD local
`D:/01Claude/ReKHCoM-build/android-avd/ReKHCoMTest.avd`; definir ANDROID_AVD_HOME al lanzarlo.
Usar app separada `com.rekhcom.android.smoke` para pruebas; nunca limpiar la app
del jugador. No publicar APK como probada en telefono por pasar emulador x86.

## Evidencia de esta iteracion

- Parser SRAM: 54 comprobaciones PASS incluyendo dos partidas privadas locales,
  sin cambios de bytes. Touch C: multitouch/diagonal/cancel/foco PASS.
- APK x86 smoke alpha2: EU/US/JP llegan a combate frame600 y terminan frame900
  sin crash. Reporte local build/android-runtime-battles/report.json. Esta prueba
  se repitio despues de incorporar 4:3 (core nuevo), con los mismos hashes de
  imagen de los tres casos. Falta ejercicio SAF en dispositivo y ARM fisico.
- 4:3 y 3:2: ejecucion Windows EU a 1280x800 en x1/x4 sin crash, capturas reales
  revisadas de combate y Estado con fuentes HD cargadas. build/aspect-runtime/.
  No equivale a probar Android/ARM, todos los zooms o todas las cinematicas.

- Resolucion RG34XX: pruebas Windows EU en 720x480, 3:2, x1/x2/x3, sin bandas
  ni deformacion del rectangulo; reporte build/aspect-runtime/rg34-resolution-report.json.
- Auditoria ABI reproducible: `python port/tools/audit_pointer_layout.py`.
  CardDef de ROM ocupa 52 bytes con punteros de 32 bits y 80 con punteros de
  64; el campo kind pasa de offset28 a56. No basta quitar el assert de Android
  ni cambiar sizeof(void*) al compilar: leeria otros bytes de la ROM.
  SaveBlockLarge sigue midiendo 0xF14 y data offset28: guardar SRAM normal
  puede seguir siendo interoperable aunque cambie el backend.
- Para ARM64/Linux real hay que separar los registros GBA (referencias u32)
  de estructuras nativas, revisar memoria DMA/allocaciones/tablas/funciones,
  o demostrar un nucleo wasm32 AOT con host SDL de 64 bits. Ninguna de esas
  rutas esta implementada; no etiquetar un ejecutable ARMv7 como ARM64.
