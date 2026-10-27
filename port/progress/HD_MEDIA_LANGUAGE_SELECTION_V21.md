# HD EN/JP: investigación y traspaso 2.1 — 27/10/2026

Tarea única: `launcher-hd-media-language-selection-v21`, cola autoritativa
`coordination/art-queue`. Investigación y primer incremento de detección/guía;
selección e imports por idioma pendientes. No
descargar contenido, cambiar idioma de Steam ni ejecutar el juego fuente en
esta fase. La entrega Windows/QA activa sigue su propio carril.

## Mecanismo comprobado y límites

La [tienda oficial de app 2552430](https://store.steampowered.com/app/2552430/KINGDOM_HEARTS_HD_1525_ReMIX/)
declara audio completo EN y JP; interfaz/subtítulos también incluyen FR/IT/DE/ES.
Esto describe el producto, no demuestra qué archivos locales están instalados.
La [ayuda oficial de idiomas](https://help.steampowered.com/en/faqs/view/4984-C127-121D-B3F2)
permite cambiar el idioma de un juego en Biblioteca → Propiedades → Idioma
(la ubicación exacta puede variar por cliente) y advierte de descargas adicionales.
La [gestión oficial de descargas](https://help.steampowered.com/en/faqs/view/71AB-698D-57EB-178C)
explica cómo esperar/controlar la cola desde Steam.

Valve distingue [interfaz, subtítulos y audio completo](https://partner.steamgames.com/doc/store/localization).
Puede distribuir idiomas juntos o mediante depots separados; no deducir el
audio a partir del idioma de interfaz, región, tienda o nombre de carpeta.
La [documentación de depots](https://partner.steamgames.com/doc/store/application/depots)
describe filtros de idioma/OS/arquitectura; no ofrece aquí una API para que un
launcher ajeno cambie el idioma/instalación de otra app del jugador.

**Ruta propuesta:** botón «Cómo instalar audio EN/JP en Steam» con tooltip y
enlaces HTTPS oficiales de ayuda/tienda. El propietario elige el idioma en
Steam, deja terminar la descarga y pulsa «Volver a comprobar». No se ha
verificado un endpoint oficial que cambie idioma y encole depots concretos;
no presentar `download_depot`, consola Steam, SteamCMD o edición ACF como
automatización soportada de este flujo. Una eventual `.bat` solo tendría sentido
como acceso a la guía/tienda HTTPS, sin cambiar Steam; no aporta ventaja frente
al botón. Un deep link al cliente requiere validación adicional antes de ofrecerlo.

Usar únicamente la instalación propia/autorizada del jugador. Ni la existencia
de un ACF ni una carpeta demuestra por sí sola licencia. No solicitar claves,
credenciales o tokens; no añadir descargas de terceros, eludir controles de
acceso ni redistribuir material del juego con ReKHCoM.

## Importador de partida y evidencia local de solo lectura

Código de partida revisado antes del incremento: `port/decompui/hd_import.inc`, `decompui.cpp`,
`port/tools/khhd_import.py`, `khhd_subs.py`, `port/src/port_hdmovie.c`.

- `FindInstall` descubre Steam mediante registro, `libraryfolders.vdf` y nombres
  conocidos de instalación. **No lee `appmanifest_2552430.acf`, `UserConfig` ni
  su `language`.** También acepta instalaciones Epic y carpetas elegidas.
- `MovieFolder` escoge `STEAM/dt/KHCReSource/BIN/movie` o el equivalente `EPIC`.
  `Recom.hed/.pkg` se lee en `Image/dt`. No hay selección de variante EN/JP.
- Importa una sola copia de cada película a `hd/movies` / `hd/scenes`; la música
  es otro componente. Los subtítulos se extraen por idioma de las tablas.
  La detección actual de texto japonés en una tabla no verifica audio JP.
- `hd/import.list` guarda ruta, calidad y tamaño; no registra idioma, depot,
  build o fingerprint de origen. **Cambiar Steam a JP y reimportar puede dejar
  EN como «ya importado»** si supera la comprobación de calidad/archivo completo.
  Requiere invalidación por procedencia, sin borrar imports buenos a ciegas.
- `MoviePath` busca el mismo nombre en escenas/películas sin idioma. El ajuste
  Undub actual usa la ROM GBA japonesa: no selecciona audio del MP4 HD y no debe
  anunciarse como equivalente a «voces HD japonesas». No hay importador separado
  de voces de gameplay Re:CoM en este alcance actual.

Inspección local de solo lectura del 27/10: appmanifest 2552430 declara
`language=spanish`, depots instalados `2552433` y `2552435`; existen
`STEAM/dt/KHCReSource/BIN/movie/ROP.mp4` y `Image/dt/Recom.hed/.pkg`.
No se abrió audio para determinar su idioma, no se cambiaron archivos y no se
descargó nada. Esto prueba instalación/paths, no «audio español» ni JP disponible.
La instalación Quiver principal permanece intocable.

Como pista secundaria, no contrato de implementación, [SteamDB depots](https://steamdb.info/app/2552430/depots/)
lista pares JP `2552432/2552434` y generales `2552433/2552435`, además de variantes
Deck `2552436/2552437`. Sus listados JP/general muestran nombres de películas
compartidos; no asumir un directorio `jp` ni coexistencia de ambas versiones.
SteamDB no es Valve: estos IDs tienen confianza orientativa y deben contrastarse
con una instalación autorizada y su manifest; no fijarlos para descargar contenido.

## Primer incremento implementado

`hd_media_source.h` añade parser VDF/ACF independiente de Win32: límite 1 MiB,
16 niveles, 8192 nodos y 4096 bytes por token; claves duplicadas, rutas de
instalación con traversal y sintaxis incompleta se rechazan. Descubre hasta 64
bibliotecas configuradas desde hasta 16 raíces y respeta `installdir` personalizado.
No escribe ACF ni Steam. La UI reusa el probe hasta cambiar carpeta o pulsar
«Volver a comprobar», sin lecturas por cada frame.

El mapping explícito se limita a app build `15194255` y exactamente estos pares
de manifests: general `2552433=2946731077053901934` /
`2552435=3908821002986173448`; JP `2552432=8878064114965627002` /
`2552434=7005798054209199771`. El general se observó localmente y JP se contrastó
con los listados secundarios enlazados; **son señales, no verificación de audio**.
Requiere instalación completa sin descarga/target build pendientes y configuración
coherente. Mezclas, revisión futura, estado incompleto y fuentes no Steam quedan
en desconocido. No se usa `spanish` para anunciar audio español.

Inventario de hasta 512 entradas, sin recursión; cada MP4 reconocido lee como
máximo 512 bytes de muestra. Se comprueban también HED/PKG sin descomprimir.
Huella FNV de rutas, revisión, idioma declarado, depots, tamaños, mtimes y muestras:
detecta cambios para refrescar evidencia, **no certifica integridad ni idioma**.
La disponibilidad es presencia legible por componente, no cobertura completa o
decodificación probada. Epic/carpetas sueltas mantienen idioma desconocido.

La UI usa `T(es,en)` y catálogo de strings; otros idiomas conservan el fallback
inglés existente hasta traducir esos textos. Ofrece enlaces HTTPS oficiales y
reescaneo. Las copias HD previas se muestran siempre con audio no verificado,
sin heredarlo de la fuente elegida. No modifica playback/import.list ni elimina
imports: la invalidación persistente y selección real van en el próximo PR.

Verificación: `python port/tools/test_hd_media_source.py` compila y ejecuta el
header de producción con fixtures ACF malformados/duplicados/profundos/grandes,
varias bibliotecas y nombre personalizado, download incompleto, revisiones/depots
mezclados, caché EN→JP y cambio de contenido con igual tamaño/mtime. Conserva el
import EN previo y prueba Epic/suelto desconocidos. Syntax-only del launcher
Windows i686 con SDL3 real pasa. No se hizo build pesado, ejecución de Steam,
descarga ni verificación auditiva/hardware/UI renderizada.

## Siguiente PR: imports y playback EN/JP

Seguimiento separado: `launcher-hd-media-language-variants-v21`, dependiente de
este primer incremento. Propuesta concreta para revisar antes de implementarlo:
`hd/media/en/{movies,scenes}` y `hd/media/ja/{movies,scenes}`, catálogo de
procedencia por archivo y ajuste independiente `hd_media_language=auto|en|ja`.
`auto` conserva el comportamiento legacy; EN/JP explícito exige esa variante
disponible y no sustituye silenciosamente por la otra voz. Si falta, UI informa
y el juego conserva el fallback del cartucho. No cambiar `game_language` ni Undub.

Publicar cada archivo/metadata con temporales y reemplazo atómico dentro de su
variante; conservar el import anterior si hay error/cancelación. Claves de cache,
posters, journal e import.list incluyen idioma, huella de fuente y calidad. Una
fuente diferente invalida el resume de ese trabajo, sin borrar material bueno.
Legacy permanece sin atribución automática de idioma. Auditar exportación HD,
packs y rutas guest/host para no perder variantes al copiar a otras plataformas.
La UI ofrecerá selección a partir del catálogo importado, no solo de la señal ACF.

1. Descubrimiento local robusto: ACF del AppID en cada biblioteca, `installdir`,
   `UserConfig/language`, depots instalados y estado de actualización como señales
   de procedencia. Formato ACF es detalle del cliente, no API estable documentada:
   parsing acotado, errores legibles y estado «no determinado». Nunca escribirlo.
2. Separar explícitamente texto/interfaz, subtítulos, voces GBA (Undub) y audio
   de vídeos/escenas HD. Mostrar EN/JP detectado, importado, faltante o desconocido
   por componente; no activar opciones solo porque exista `ROP.mp4`.
3. Comparar archivos/metadata y fingerprint de origen. Idioma declarado por
   Steam es una pista; archivos homónimos y pistas MP4 sin tag fiable requieren
   validación adicional. Mantener desconocido antes que inventar disponibilidad.
4. Permitir elegir EN/JP solo con material importado y validado. Definir namespace
   por idioma o catálogo de variantes antes de tocar playback; conservar el
   namespace legacy como import desconocido/migrable. Journal, import.list,
   posters/previews y cachés deben incluir procedencia/idioma para evitar mezclar.
5. Guía Steam y reescaneo manual; advertir que cambiar idioma puede sustituir
   archivos fuente. No prometer descargar ambas variantes simultáneamente. La
   obtención e importación de cada variante necesita validación real del flujo.

Prueba mínima siguiente: fixtures ACF EN/JP/ausente/malformado, varias bibliotecas,
descarga incompleta, fuentes homónimas, import existente EN seguido de fuente JP,
cancelación y conservación del import previo. Después, un propietario verifica
una película hablada y una escena en ambos idiomas con subtítulos independientes;
sin requerir campaña completa ni tocar partidas. Probar también UI localizada,
desconocido/faltante, off/on y fallback al cartucho sin saltar historia.

La afirmación histórica «PC solo trae audio inglés» en `HD_MOD.md` se corrigió
para describir solo la fuente inspeccionada entonces; no generalizarla al producto
actual con audio JP. No modificar esas copias antiguas para acomodar la señal.
