# Oráculos funcionales y composición de código reconstruido

Cargar sólo al diseñar o revisar pruebas diferenciales de código recuperado. Leer primero [contratos de equivalencia](decompilation-contracts.md); para datos, copia de código y dispositivos GBA, [cierre GBA](gba-closing.md). No añadir este flujo a una aplicación ordinaria que sólo necesita sus pruebas habituales.

## Impedir que el oráculo acepte un candidato que no ejecutó

Fijar hash del original, compilador/opciones, entradas, regiones mapeadas y contratos adaptados. Ejecutar original y candidato en estados iniciales equivalentes. Si se permite llamar a dependencias originales, enumerarlas: prohibir durante la ejecución del candidato la entrada al código que se está reconstruyendo, incluidas sus copias ejecutables en RAM. Un callback o wrapper que termina llamando al ASM objetivo puede producir un falso pase por forwarding.

Un límite de instrucciones es una protección, no una prueba de retorno. Comprobar que cada llamada alcanza el sentinel de salida y restaura SP según su contrato. Comparar retornos pertinentes, memoria afectable completa y canarios, además de bytes de registros y escrituras I/O ordenadas cuando formen parte del resultado. Si se excluye stack temporal, workspace nuevo o campos de bindings, fijar rangos y razón antes del resultado; no excluir una diferencia desconocida para conseguir igualdad.

Probar controles negativos que el harness deba rechazar. Uno especialmente útil es un candidato que limpia el payload y después nunca retorna: una API de emulación puede detenerse al agotar instrucciones sin lanzar error. Comparar sólo la ventana esperada podría dejarlo pasar pese a que no retornó. Usar patrones no cero, verificar regiones externas al payload y exigir PC/SP. Añadir un control de escritura fuera de rango cuando el contrato lo permita. Estos controles validan el harness, no la equivalencia universal del programa.

Separar resultados: C funcional bajo entradas acotadas, bytes/relocaciones aislados, artefacto integrado, ABI original y dispositivo/timing. Mantener como diagnóstico una coincidencia obtenida con otro compilador o flag no aprobado; no cambiar el toolchain del producto para convertirla en progreso aceptado. Ejecutar ARM con direcciones serializadas32 tampoco demuestra un port host64.

## De APIs sueltas a motor compuesto

Una suite por función no acredita la composición. Crear escenarios sintéticos persistentes que unan las entradas reales, callbacks y consumidores; para un motor de audio, secuenciador → asignación de notas → mixer → sincronización durante varios frames. Conservar RAM entre llamadas y comparar en fronteras que localicen el primer frame/fase divergente: PCM producido, estado persistente, registros y orden de I/O. Registrar hash fuente/candidato y si la compilación fue fresca.

Preparar la tabla/callbacks mediante el código de binding que se pretende entregar. Reescribir slots desde Python puede servir para explorar, pero no prueba ese wiring C. Probar bindings conocidos, idempotencia, overrides conservados y cadenas; documentar las convenciones especiales o overrides desconocidos que aún requieren adaptador. Canonicalizar sólo campos de dirección declarados, sin sustituir indiscriminadamente enteros que parecen punteros.

Exigir señales de activación: no basta llamar a un escenario «loop», «fade» o «LFO». Comprobar notas repetidas, cambio de estado, dueño del canal tras prioridad, parada real y salida no vacía según el caso. Incluir fin de muestra/nota, bucles, robo de canales y tamaños de buffer pertinentes. El número de frames o casos no se convierte en porcentaje de equivalencia. I/O modelado y PCM igual no certifican audio real, DMA ni presupuesto de ciclos.

## Evidencia y revisión de estas lecciones

Ledger revisado el 2026-10-28. Las cifras de abajo describen casos concretos, no objetivos ni presupuestos transferibles. No se distribuyen ROM, glifos, paletas o PCM del juego.

| Mecanismo | Fuente y estado | Límite |
|---|---|---|
| Retorno real, PC/SP, EWRAM y rechazo de forwarding | [SS2 PR28 merged](https://github.com/MatyMeatBoy/SS2Decomp/pull/28), [harness fijado](https://github.com/MatyMeatBoy/SS2Decomp/blob/f61d53560768e07a44c266189d4a9c524bfb71c1/research/m4a_1/test_helpers.py) y [contratos](https://github.com/MatyMeatBoy/SS2Decomp/blob/f61d53560768e07a44c266189d4a9c524bfb71c1/research/m4a_1/README.md). La revisión pública informa que el control Clear64 infinito pasaba antes y fue rechazado tras exigir retorno/pila y memoria completa. | PR28 publica helpers/Note y326 casos; no atribuirle el motor completo posterior. |
| Copia ejecutable y nombres por evidencia | [Guía SDK fijada](https://github.com/MatyMeatBoy/SS2Decomp/blob/f61d53560768e07a44c266189d4a9c524bfb71c1/docs/agents/SDK_M4A_IDENTIFICATION.md). Copia IWRAM1024B frente a intervalo de símbolo932B. | Auditar rango copiado y consumidores; el límite del símbolo no define por sí solo lo ejecutado/copiado. |
| Composición persistente, C binders y guard ROM/IWRAM | [SS2 PR29 integrado](https://github.com/MatyMeatBoy/SS2Decomp/pull/29), [escenarios fijados](https://github.com/MatyMeatBoy/SS2Decomp/blob/0fed377ba72198198468238647333649254e93da/research/m4a_1/test_engine.py) y [harness por API](https://github.com/MatyMeatBoy/SS2Decomp/blob/0fed377ba72198198468238647333649254e93da/research/m4a_1/test_helpers.py). Reproducido localmente y repetido por integrador:12 escenarios×32frames,768 fronteras Main/VSync;855 casos por API y10 checks de binding. | 0/35 intervalos ELF configurados iguales; pueden contener entradas compartidas y pools, no son necesariamente35 rutinas semánticas. Cero crédito matching. Se excluyen20B de servicios y1KiB de stack, con campos de callback canonicalizados. No prueba ABI, hardware ni audio. |
| Revisión histórica, gitlink y nombres auditados | [C histórico](https://github.com/Dream-Atelier/kl-eod-decomp/blob/6617baf0e2024da1c1d08f6fccde017a32df1cdb/src/m4a_1.c), [gitlink del compilador](https://api.github.com/repos/Dream-Atelier/kl-eod-decomp/git/trees/6617baf0e2024da1c1d08f6fccde017a32df1cdb?recursive=1), [compilador fijado](https://github.com/Dream-Atelier/agbcc/tree/1b1c4a81d13918ca55c39c1ccf83689691b03a25). Investigación local: VoiceLookupAndApply corresponde a ply_fine por cuerpo/callers, no por su nombre. | Con compilador privado y -ftst:46B+relocs iguales; toolchain SS2:48B distintos. -ftst no pertenece a la configuración aceptada. GPL2 del compilador no concede licencia al donante C; no se copió fuente de licencia incierta. |
| Decoder externo frente al consumidor | [Extractor fijado](https://github.com/foone/ShiningSoul2Extractor/blob/8b230e6a9d1b56feda994fa8a2e9a9c996e20caf/extract_ss2.py), [avisos/licencia](https://github.com/foone/ShiningSoul2Extractor/blob/8b230e6a9d1b56feda994fa8a2e9a9c996e20caf/LICENSE). Investigación local del consumidor EU, aún sin publicar su receta:153 glifos exportados frente a256 slots de128B; índice=(byte−1)&255 y copia64B por glifo. | Los consumidores prueban stride/rango, no Unicode ni uso de todos los slots. El extractor público no acredita por sí solo los hallazgos locales del consumidor. Round trip del rango concreto no equivale a porcentaje global de datos entendidos. |

Separar en el ledger fuentes públicas y observaciones locales. Una fuente pública del decoder no convierte una auditoría local del consumidor en evidencia públicamente reproducible; publicar la receta verificable antes de afirmar esto último.

## Casos antes/después de esta actualización

Revisión manual de decisiones, sin subagentes ni evaluación ciega. Antes ya existían las distinciones funcional/matching/hardware y denominadores; faltaban controles concretos de forwarding/retorno y composición. No se atribuye una mejora medida del comportamiento de agentes a esta edición documental.

| Entrada representativa | Insuficiencia anterior específica | Decisión exigida ahora |
|---|---|---|
| Clear64 limpia64B y queda en bucle con límite de instrucciones | El comparador estricto de objetos no detecta que un harness funcional aceptó falta de retorno | Rechazar por PC/SP y probar canarios/memoria; control negativo público PR28 |
| C pasa porque su callback entra al target ASM copiado en RAM | «Oráculo reproducible» no explicitaba el forwarding mediante copia ejecutable | Guard de entrada para ROM y todas las copias objetivo; dependencias permitidas enumeradas |
| Helpers verdes, slots reparados desde Python y estado nuevo cada llamada | No había gate concreto para composición persistente/binding entregable | C binders reales, frames persistentes, activación y primer fallo por fase |
| Decoder publica153 elementos, consumidor admite256 slots | Ya se pedían consumidores; faltaba separar límite del extractor de capacidad probada | Reabrir/reconstruir el rango acotado, conservar diferencias y no extrapolar porcentajes |
| C coincide sólo con gitlink histórico y -ftst | Ya se pedían flags; faltaba impedir promoción de ese diagnóstico a toolchain aceptado | Registrar revisión/flag y nombres por cuerpo, mantener el resultado fuera de matching integrado |
| Aplicación web sin recuperación binaria | Riesgo de cargar detalles de GBA fuera de contexto | No cargar esta referencia ni sus controles de plataforma |

Controles ejecutados en salida aislada durante esta actualización, con C propio compilado por agbcc y Unicorn: ClearGood pasó ventana96B, retorno/SP y EWRAM completa; ClearNoReturn limpió64B y agotó10000 instrucciones, pasando la ventana antigua pero fallando el sentinel; ClearFarWrite limpió64B y escribió a+4096, pasando ventana/retorno pero fallando EWRAM completa. Son controles sintéticos reproducidos del mecanismo, sin ROM ni assets, no una evaluación independiente de agentes. Composición y controles por API proceden de la investigación local separada indicada en el ledger.
