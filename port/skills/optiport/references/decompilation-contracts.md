# Recuperación de fuente y contratos de equivalencia

Leer sólo al reconstruir código desde binarios, adaptar un runtime recuperado o evaluar un decompilado. OptiPort sigue siendo un método de mejora de software general; una aplicación con fuentes disponibles no necesita adoptar este flujo por usar una biblioteca nativa. Para buscar donantes, usar [investigación de repositorios](repository-research.md); para trabajo paralelo autorizado, [coordinación](coordination-workspaces.md).

Si el producto es GBA y el trabajo trata su cierre, nombres, datos o port, cargar [cierre y portabilidad GBA](gba-closing.md). No aplicar sus reglas de hardware y formato a otros perfiles.

## Fijar qué se acepta

| Objetivo | Evidencia necesaria y límite |
|---|---|
| Matching | C/C++ en contexto real, toolchain/opciones fijadas, bytes y relocaciones correctos y artefacto final según contrato del proyecto. |
| Recuperación funcional o port | Oráculo reproducible del original, ABI, resultados, efectos y errores relevantes. Pruebas acotadas no demuestran equivalencia universal. |
| Identificación/documentación | Rol, nombre propuesto, tipos y grafo con evidencia y confianza; contabilizar aparte de código aceptado. |

Registrar hash original, módulos/rangos únicos, configuración y comando de aceptación antes de experimentar. Usar identidad estable como hash/módulo+RVA, no sólo nombres cambiantes. No inferir tamaño total de código del tamaño del archivo: recursos, padding, thunks, fragmentos y código compartido pueden alterar el inventario.

Inspeccionar formato y componentes antes de elegir herramientas: PE/ELF/Mach-O, IL, AOT, bundle o firmware. Un launcher sin directorio CLR no descarta ensamblados .NET acompañantes. Confirmar ABI, convención de llamada, endian, widths, alineación, imports y runtime por módulo; x64 Windows no hereda reglas SysV. Consultar documentación primaria cuando cambie una decisión, sin asumir un compilador por el año del producto.

## Del ASM/IL al candidato

Leer instrucciones y callers antes de convertir pseudocódigo en una firma. El decompilador, incluido m2c cuando sea aplicable, propone una base: verificar signo, anchura de loads/extensiones, truncamientos, retorno, varargs, offsets y layouts. Comprobar `sizeof/offsetof` con el compilador objetivo. Un header moderno o el ancho de puntero host puede cambiar código y ABI aunque el C compile.

Recuperar CFG y tablas de salto completos antes de dividir funciones grandes. Bloques internos, tail calls y datos pueden estar mal clasificados como funciones o llamadas; una exportación truncada no define el alcance real. Contrastar destinos y accesos con el binario. No introducir helpers para facilitar reparto si matching exige conservar optimización global y layout.

Iterar una hipótesis observable por experimento: tipo, estructura del bucle, orden, lifetime, pool o revisión SDK. Compilar primero; variantes/permuter sirven para diferencias pequeñas y presupuestos acotados, no para sustituir una hipótesis ABI. Guardar intentos fallidos y siguiente prueba mínima. Checkpoint significa preservar, no matar/reiniciar un proceso útil por reloj.

## Auditar el comparador y el contexto

Un score o diff relajado sirve para priorizar. No ordenar instrucciones ni borrar ramas/destinos, opcodes, addends, registros o identidades de sección para declarar igualdad. En ELF REL parte del addend vive en la instrucción; normalizar toda la palabra puede esconder cambios reales. Una relocation a otra sección no es una referencia local intercambiable.

Antes de automatizar aplicaciones, probar el verificador con diferencias deliberadas: orden, rama, addend, registro de salto y símbolo/sección distintos. Verificar tanto negativos que debe rechazar como cambios de ubicación permitidos. Conservar las exclusiones legítimas fijadas por el contrato; no ampliarlas después de un fallo. Para ELF ARM consultar [AAELF32, addends y PC bias](https://github.com/ARM-software/abi-aa/blob/2025Q1/aaelf32/aaelf32.rst#5611-addends-and-pc-bias-compensation); otras arquitecturas requieren sus reglas.

Aliases de símbolo/tipo pueden resolver declaraciones incompatibles sin reescribir headers compartidos; comprobar el símbolo efectivamente emitido, orden y visibilidad de declaraciones y contexto completo del TU. Un match de función aislada no valida datos externos ni sustituye reconstrucción final. Aplicar reversiblemente, construir el módulo y artefacto, ejecutar gates pertinentes y conservar el último aceptado. ASM/original usado en build normal demuestra integridad de la base, no equivalencia del borrador que queda detrás de una condición.

Para diseñar un harness funcional o validar composición, cargar [oráculos protegidos y escenarios persistentes](oracle-validation.md). Incluye controles negativos de retorno/forwarding y límites de publicación; no es necesario para una comparación de objetos aislada.

## SDK y runtime: reutilizar sin inflar resultados

Inventariar primero C ya recuperado, borradores, biblioteca enlazada y ASM original escrito a mano. Respetar las exclusiones justificadas del proyecto: no convertir un mixer programado a mano a C como obligación nueva. Un objeto runtime puede proceder de C original y seguir siendo `linked`; cambiar la etiqueta no es un logro de decompilación.

Comparar revisión SDK, constantes, estructuras, ABI y opciones reales. El fuente de un runtime puede contener copias distintas para arquitecturas o etapas del compilador. Reproducir el miembro correcto del archive en salida propia, comparando bytes/relocaciones y datos asociados cuando importen. Una implementación `/` de alto nivel no sustituye automáticamente un helper de división con convención particular.

Un caso comprobado enseñó que una variante antigua del driver difería de una copia moderna por una lectura volatile, un estrechamiento u8 y una actualización de envolvente; la variante compatible pasó comparación aislada y después reconstrucción integral. La lección es buscar revisiones y contrastar esos efectos, **no borrar lecturas ni insertar casts en otros drivers**. Fuentes primarias para estudiar el mecanismo: [driver del payload histórico](https://github.com/pret/colosseum-mb/blob/65ef452784d15091028c202b85fe05bdae0ae690/payload/src/m4a.c), [fuente y receta original del runtime](https://github.com/pret/agbcc/tree/da598c1d918402c42c0c0d7128ba14567f3175e9/libgcc). Disponibilidad pública no sustituye revisar licencias/avisos.

## Métricas y entrega

Publicar por separado bytes únicos y funciones aceptadas; borradores, linked, ASM retenido, nombres confirmados/propuestos, aristas indirectas sin resolver y datos crudos/decodificados tienen contadores distintos. No sumar rangos solapados ni blobs copiados como datos comprendidos. Con denominador desconocido, publicar cantidades y cobertura conocida, no porcentaje global. En funcional, cobertura de pruebas no es porcentaje recuperado.

Un gigante puede tener hitos enumerados y documentados sin estar matching. No convertir proporción de líneas, tamaño parecido o regiones revisadas en porcentaje de equivalencia. Comparar rendimiento de workers sólo entre tareas comparables, registrando tiempo real, revisión, rechazos y producción integrada. Modelos o concurrencia mejores en una muestra no garantizan velocidades en otro software.

Entregar revisión, función/rango, fuentes, comandos/resultados, modo realmente probado, descartes y siguiente acción; diferenciar aislado, integrado, publicado y confirmado. Las rutas, cifras, tickets y estado temporal viven en el proyecto, no en esta skill. Para revisar las lecciones y casos de aplicación, ver [validación de este módulo](decompilation-validation.md).
