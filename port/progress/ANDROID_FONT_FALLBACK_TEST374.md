# Prueba Android de fuentes y arranque: #374 / #412

El 28/10/2026 se publicó y envió la [prueba v2.2-test374-font-fallback](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.2-test374-font-fallback). Es una prerelease inmutable; Latest sigue en v2.2. Cierra la entrega del candidato, no la confirmación física de ninguno de los issues.

## Artefacto y procedencia

- APK: `ReKHCoM-v2.2-test374-font-fallback.apk`, 27 542 496 bytes.
- SHA256: `f1e74a7589c6b3e5456cbf8b543f8fb06a867f2d572622a8272c6ba11bc1d630`.
- Fuente de empaquetado/tag: `2dd3d01bafc071cf2959e27bc04ba762e24190ed` (#468).
- Fuente nativa declarada: `60c24a89579f72c6e25f6671e4ead26905131058` (#467), con #465/#466 incluidos. Se recompilaron los tres módulos FreeType de Android32 y sus bibliotecas EU/US/JP.
- Guests Android64 producidos en `7a4eea073641ae658c5ad6c63cb7cebdd1b99fa9`: conservaron exactamente sus hashes al incorporar #467, que no entra en ese productor. También quedó idéntico `librekhcom64.so`.
- Ambos launchers compilaron la unidad C++ completa desde `d5107162cfb67a396ebc17656f6baedd6489a596`; su blob coincide con #466 integrado y con la fuente final. Los hashes permanecieron iguales durante el refresco FreeType.

El APK universal contiene cinco bibliotecas ARM32 (`armeabi-v7a`) y tres ARM64 (`arm64-v8a`), con los ELF comprobados. Conserva `com.rekhcom.android`, versionCode `102020099` y certificado SHA256 `3a5a1740e8171c82b505907da26f3039e2153aeb8d16c2c22477d52edf74ecdc`, para permitir actualización sin desinstalar ni borrar datos. El nombre de versión es nuevo. No contiene ROM, SAV ni configuración personal.

## Qué demuestra y qué falta

Cinco tombstones de #374 identifican el acceso inválido de `ImGui::NewFrame` a un atlas incompleto. [El fix compartido](LAUNCHER_FONT_ATLAS_374.md) valida el atlas antes de consumirlo, intenta una fuente incorporada si falla y sale con diagnóstico si también falla ese intento. El archivo de fuente particular que falló en el Motorola sigue sin identificarse. La entrada Java y el acceso directo al juego de [#462](ANDROID_SAFE_STARTUP_TEST.md) complementan ese fix; no se presenta únicamente el bypass como solución causal.

Las baterías CPU observadas son independientes: 16 casos del consumidor ImGui, 44 del inicio/limpieza, 12 del reemplazo de fuente ajustada y 20 de rasterización/subida de glifos. Además pasaron la compilación completa de los launchers Android32/64 y los controles de empaquetado, firma, identidad, ocho ELF y hashes de assets. Estos resultados no equivalen a una instalación o ejecución Android.

**Instalación, runtime y resultado físico Motorola/API36: UNVERIFIED.** Tres intentos con el AVD API34 existente fallaron antes de instalar el candidato; se conservaron sus registros y se detuvieron los reintentos. No certifican ni refutan el comportamiento del APK en el teléfono. Tampoco demuestran una causa común con S22 o Steam Deck330. El fallback incorporado puede perder tipografía personalizada o cobertura japonesa.

## Entrega y continuidad

La revisión pública verificó los tres assets y sus digests, tag y prerelease. Se enviaron respuestas separadas a [#374](https://github.com/MatyMeatBoy/ReKHCoM/issues/374#issuecomment-6480702936) y [#412](https://github.com/MatyMeatBoy/ReKHCoM/issues/412#issuecomment-6480703814). Falta el resultado del reportero; no cerrar como «Moto/S22 arreglado» por publicar el APK.

La baseline privada `2.2-test374-safe-startup`, SHA256 `a68b0ec219ec3a8ab07be2b0dde240790ca8842bc92f4a07ad49fb072de20be7`, se verificó sin cambios y no se publicó. Los recibos de entrega y auditoría están en `releases/v2.2-test374-font-fallback/` y `pruebas/moto374-safe-startup-current/`; no se usa mtime como prueba de compilación. El APK, los metadatos y los assets publicados no se regeneran: cualquier evidencia posterior será un suplemento nuevo.
