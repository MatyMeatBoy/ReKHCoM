# Cierre, nombres y portabilidad de software GBA

Usar sólo para un proyecto GBA de reconstrucción o port. Leer primero el contrato de aceptación de [decompilación](decompilation-contracts.md); esta referencia no impone finalizar datos o un port cuando el usuario sólo pide una función. Mantener estas dimensiones separadas:

| Dimensión | Evidencia y frontera |
|---|---|
| Código matching | Artefacto reconstruido con toolchain/opciones y hash esperados; inventario de C, runtime linked y ASM original retenido. |
| Nombres y comprensión | Roles/layout con evidencia por identidad canónica; los nombres por slot de tabla no prueban rol semántico. |
| Datos y reconstrucción | Estructura/punteros documentados, extracción reproducible de entradas propias y gates de build; un blob copiado no es dato comprendido. |
| Relocatabilidad | Punteros/branches correctos al mover regiones bajo un contrato explícito; el hash matching del layout original no la demuestra. |
| Port funcional | Contratos de ABI, formato, memoria, entradas/salidas y scripts pertinentes en host; compilar no acredita compatibilidad. |
| Emulador y hardware | Core/BIOS/configuración/script/dispositivo identificados; éxito emulado no certifica timing físico, link ni caminos no ejecutados. |

Un badge100% puede medir sólo la primera fila o incluir ASM/bibliotecas originales. Conservar denominadores y exclusiones en el proyecto. No retirar ASM escrito a mano ni reclasificar linked para aumentar una cifra.

## Cerrar el build sin cerrar otras preguntas artificialmente

Fijar región/revisión y hash de entrada, toolchain, receta de extracción y hash del artefacto. Comprobar desde una salida limpia que los generadores producen lo esperado. Separar fuente canónica y archivos generados: editar la configuración/tabla propietaria y regenerar, no corregir un resultado generado a mano. No cambiar el hash esperado para esconder un fallo.

Inventariar explícitamente las excepciones justificadas: startup, cambio de modo/pila, mixer manual, thunks BIOS y runtime, aliases o restricciones de codegen. Explicar su origen y qué sigue ASM. Revisar qué hace realmente CI: puede sustituir assets por ceros y verificar sólo el resto. Para una entrega registrar SHA fuente, hashes, versiones, pruebas ejecutadas, entradas que el usuario aporta, licencias/avisos y límites de emulador/dispositivo. Fuente y CI no implican que exista una release empaquetada.

## Nombrar con evidencia y cambios revisables

Mantener catálogo por hash/módulo+dirección canónica o RVA, conservando símbolos antiguos, nombre propuesto/confirmado, confianza, evidencia y revisión. Usar el rol verdadero en los caminos/callers/readers/writers relevantes; un nombre como Init, Clear o All afirma un alcance que debe comprobarse. Una función homónima de otro motor sólo aporta evidencia si coinciden papel, forma, argumentos y usos. Conservar incógnitas cuando no hay prueba suficiente.

Separar nombres semánticos de identidad por posición en una tabla probada por su consumidor. Revisar alcance y referencias compartidas antes de nombrar un registro por dueño; una celda scratch reutilizada puede tener varios roles. No imponer vocabulario de otro producto ni convertir nombres inferidos en documentación confirmada.

Aplicar lotes pequeños de **sólo renombrado**, sin cambios simultáneos de prototipos, tipos o lógica. Dry-run de colisiones, actualización canónica, regeneración, diff que revierte aliases a los nombres antiguos y build/hash antes/después. Conservar registro old→new para documentación histórica. Renombrar un campo por struct/offset y accesos tipados, no por reemplazo global de `unk14` que pertenece a varias estructuras. El propietario del catálogo coordina los cambios; shards de código no renombran todo el proyecto.

## Datos: estructura probada y extracción reproducible

Separar contenido protegido de metadata estructural y cumplir la política/licencias del proyecto. Una receta puede publicar offsets, tamaños, tipos y referencias simbólicas mientras extrae assets del dump propio durante el build. Eso permite reconstruir sin publicar bytes, pero no implica que todos los assets estén decodificados o comprendidos. Validar rangos/solapamientos y round trips cuando haya un formato extraído/regenerado.

Probar límites y campos de tablas desde consumidores, formato y referencias. Una palabra dentro del rango ROM puede ser número coincidente, dirección, código o datos. No convertirla automáticamente a puntero ni justificar extensión por la siguiente etiqueta sin revisar. Thumb/ARM y destinos de jump tables pueden requerir tratamiento distinto del bit de estado; fijar la semántica del consumidor antes de simbolizar.

Un pipeline de traslado puede combinar: inventario estático de punteros/branches, enlace con padding en puntos seleccionados y ejecución determinista original/trasladado. Documentar clasificación de palabras sospechosas y desconocidas, normalización permitida de direcciones y cobertura del script. Los stand-ins de un fixture pueden resolver símbolos ausentes en el link real: conservar el gate integral. No ejecutar traslados en la instalación protegida del usuario.

## Port host: puntero serializado y puntero nativo

Los streams GBA pueden guardar direcciones32 poco alineadas; un host64 no puede reinterpretarlos como punteros nativos o structs ensanchadas. Identificar cada frontera: headers/tablas regenerados con punteros reales, byte streams que conservan ancho y endian, y un lector/traductor central de sus operandos. Auditar GOTO/PATT/REPT, comandos condicionales y referencias a samples que usen esa representación; un lector nuevo no cubre las rutas que aún hacen casts directos.

Un helper inline o macro situado antes de una función SDK puede ser la diferencia decisiva de codegen. Leer definición completa y opciones antes de descartar una revisión compatible. Un cast entero32 puede preservar el patrón original GBA y truncar un puntero host64: mantenerlo confinado a su ABI o adaptar la representación con pruebas del port. No enseñar un cast específico como parche general.

Para audio distinguir buffer del mixer, registros PSG, fase de timers/DMA y audio renderizado. El mismo buffer no prueba el mismo instante de escritura; mismo valor PSG por frame puede ocultar diferencias entre frames. Un mixer C puede necesitar modo ARM, residencia IWRAM, bloque copiable acotado y branches relativos para cumplir presupuesto GBA; comprobar tamaños/limites propios antes de copiar esa estrategia. No reclamar matching por una implementación funcional del mixer original ASM.

Al comparar audio fijar tasa, formato, canales, reloj, input y opciones del emulador. Un modo sin wait states ayuda a una pregunta funcional pero altera el contrato de timing. Declarar diferencias de samples y rutas no ejecutadas; no tolerarlas silenciosamente porque el audio parece correcto. Para hardware considerar BIOS, cartucho, wait states, cruces de bus, suspend/resume y link según alcance. No anunciar prueba de hardware por un harness de un solo core con HLE BIOS.

## Procedencia y límites de estas lecciones

Se revisaron fuentes completas, no se ejecutaron sus ROMs ni sus tests aquí. Son patrones para decidir pruebas propias, no certificaciones de estos productos:

- [knidl: nombres y registro de evidencia](https://github.com/overjt/knidl/blob/628f4d461227d7a9e53b02e9eac8f63f4db5cf8b/docs/naming.md), [audit de ASM/excepciones](https://github.com/overjt/knidl/blob/628f4d461227d7a9e53b02e9eac8f63f4db5cf8b/docs/audit.md) y [datos, shift y boot tests con exclusiones](https://github.com/overjt/knidl/blob/628f4d461227d7a9e53b02e9eac8f63f4db5cf8b/docs/data.md).
- [heat2002: harness de traslado](https://github.com/shohamc1/heat2002/blob/5b9ed567e2129d38c0b18e0135c83c453a038862/scripts/shift_test.py): compara escenas/memoria y datos usados; no prueba caminos ausentes.
- [driver histórico e inline de acceso a tracks](https://github.com/pret/colosseum-mb/blob/65ef452784d15091028c202b85fe05bdae0ae690/payload/src/m4a.c): fuente de comparación matching, no permiso para copiar sin revisar licencia.
- [FE7: fronteras de datos del port](https://github.com/7ucina/fireemblem7u/blob/cc25be0657bc83be7dff8c8c248967e068df0b65/docs/port-data.md), [mixer C](https://github.com/7ucina/fireemblem7u/blob/cc25be0657bc83be7dff8c8c248967e068df0b65/src/m4a_mixer.c) y [harness de audio](https://github.com/7ucina/fireemblem7u/blob/cc25be0657bc83be7dff8c8c248967e068df0b65/tools/emutest.py): NONMATCHING/host separados del build ASM matching. Su disponibilidad pública no establece una licencia genérica ni equivalencia con otra revisión SDK.

Casos revisados al añadir la referencia: (1) lote de nombres sin cambio de bytes acepta sólo conocimiento sustentado, no aumenta decomp; (2) host64 con dirección32 requiere traducción, no cast nativo; (3) audio igual sólo en modo rápido no acredita timing; (4) code100% con datos no medidos mantiene pendiente ese eje; (5) servicio web no carga este módulo. Son controles de decisión manuales, no builds ni benchmark independiente. Cifras, paths y pendientes concretos se guardan en el proyecto.
