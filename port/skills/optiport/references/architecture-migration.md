# Migración de 32 a 64 bits

Buscar compatibilidad y beneficios medidos, sin asumir que 64 bits será más rápido. Inferir primero la matriz viable desde el código, generación de código, dependencias y toolchains disponibles; una CPU de 64 bits no demuestra soporte del sistema operativo, bibliotecas o aplicación.

## Límites que deben conservarse

| Área | Comprobación útil |
| --- | --- |
| Modelo de datos | Comparar ILP32, LP64 y LLP64: ancho de `int`, `long`, punteros y estructuras. Revisar casts, truncamiento, aritmética de direcciones, formatos de impresión y alineación. No serializar estructuras nativas como sustituto del formato original. |
| Direcciones del software | Un host de 64 bits puede conservar direcciones serializadas de 32 bits cuando el contrato de formato binario, código generado o memoria emulada lo exige. Separar explícitamente punteros host de direcciones/offsets del software y comprobar sus conversiones. |
| Archivos y documentos | Mantener anchos, endian, layout y versiones de formatos legacy. Probar lectura y escritura cruzadas entre builds con copias de documentos; verificar también cachés y código generado existente. |
| ABI y callbacks | Auditar Microsoft x64 frente a SysV AMD64 donde corresponda: argumentos enteros/flotantes, retorno, alineación de stack, shadow space/red zone y registros no volátiles. Probar wrappers, trampolines y callbacks con tipos reales, incluidos floats; una llamada de enteros que pasa no valida toda la frontera. |
| Dependencias | Ejecutable, DLL/shared libraries y plugins de cada proceso deben tener arquitectura y ABI coherentes. Revisar disponibilidad de decoders, renderer, audio y funciones HD, además del enlace correcto. |
| Rutas efectivas | Revisar preprocesado, flags y capacidades negociadas: una función puede compilar en el build de 32 bits y desaparecer en un guest freestanding de 64 bits. Registrar el renderer real, calidad efectiva y motivo de fallback; un nombre fijo de backend o una compilación exitosa no demuestran paridad. |

## Selección y fallback

Si se solicita un paquete Windows x86/x64 único, incluir ambos ejecutables con dependencias separadas y selección **Auto / Forzar 32 bits / Forzar 64 bits**; aplicar el mismo diseño en Linux cuando corresponda. El selector comprueba compatibilidad antes de iniciar y explica una selección no disponible. Auto no prioriza 64 bits sin paridad de funciones, compatibilidad de guardados y evidencia A/B. Probar argumentos, rutas, guardados, logs y terminación del proceso hijo. El fallback a 32 bits debe ser una decisión segura al inicio, con motivo visible; no relanzar el software automáticamente tras un crash durante la sesión.

En Android, un APK universal sólo incluye las ABI que el proyecto puede construir y soportar. Android selecciona la ABI del proceso según el sistema; un selector dentro de ese proceso no puede cambiarlo a 32 bits. No fingir compatibilidad ni ofrecer un force que intente cargar código incompatible. Probar instalación, arranque y paridad por ABI en dispositivos/sistemas reales cuando estén disponibles.

## Evidencia para recomendar una arquitectura

Comparar A/B con el mismo escenario, assets, opciones, backend y calidad: carga cold/warm, regularidad de cuadros y picos de memoria. Los punteros, estructuras y cachés pueden ocupar más RAM en 64 bits. Exigir paridad de funciones, incluido HD cuando exista, antes de atribuir una mejora a la arquitectura.

Compilar con el SDK y los guards reales de cada frontend afectado. Un escáner textual de constantes o un harness del motor puede pasar mientras una UI compartida invoca una función disponible sólo en escritorio. Mantener funciones de extracción/reparación separadas de receptores móviles y probar el punto común que las llama; no inventar respuestas de éxito para ocultar una API ausente.

Registrar por separado compilación, probes de ABI, harness, arranque del paquete y recorrido real del software. Un probe SysV aprobado no certifica Microsoft x64 ni una aplicación completa. Mantener pendientes explícitos de QA, hardware y sesiones prolongadas; publicar una alpha sólo dentro de la autorización del usuario, con SHA, hashes y límites comprobados.
