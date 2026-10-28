# Coordinación y workspace liviano

Aplicar cuando haya trabajo paralelo autorizado o worktrees que gestionar. Reutilizar la autoridad de pendientes existente; no crear una segunda cola para repetirla. Un snapshot manual informa, pero no sustituye una cola con ownership actualizado. Roles y herramientas concretos pertenecen a la documentación del proyecto, no a una regla universal de OptiPort.

Para reunir carpetas, distinguir releases de pruebas, facilitar la identificación humana y reducir uso real de disco o VM, aplicar [organización y almacenamiento](workspace-storage.md). No confundir archivar un checkout con liberar todo el tamaño que muestra una búsqueda recursiva.

## Claim, evidencia e integración

Antes de editar, comprobar la autoridad y reclamar la tarea mediante su mecanismo atómico/CAS (compare-and-swap) si existe. Registrar dueño, alcance/archivos, dependencias y branch; si otro claim ganó, replanificar sin pisarlo. Sin CAS disponible, usar coordinación explícita y evitar solapamiento, sin fingir exclusión atómica. Un lock de integración serializa al integrador; no concede propiedad sobre el trabajo ajeno.

Mantener en la tarea evidencia compacta: SHA fuente probado, pruebas y resultado, PR, merge, artefacto/versión cuando aplique y próxima prueba mínima. Separar estos estados: editar, compilar, revisar, integrar y entregar no son equivalentes. Revisar de forma independiente lo que el riesgo requiera. Tras integrar, probar la combinación final con su SHA; compilar un fixture o fuente previa no valida el resultado combinado.

Si un rebase/merge falla, detener pruebas y pushes que dependan de esa operación hasta resolverlo y confirmar el estado final. No continuar sobre una premisa de actualización fallida. Cachés bare por worker pueden reducir transferencias/historial repetido, pero el claim/CAS debe refrescar la autoridad antes de decidir ownership; una caché desactualizada no decide quién puede editar.

Delegar sólo dentro de la autorización: pregunta acotada, contexto mínimo, restricciones, archivos/dependencias y evidencia esperada; conservar modelo/esfuerzo autorizados. Compartir resultados dentro del trabajo no autoriza comunicaciones externas, comentarios ni cierres de issues.

Separar el pico/límite del árbol de procesos propio de la memoria física libre, que puede variar por cargas ajenas. Si un umbral externo cancela repetidamente el mismo trabajo, conservar cachés y checkpoints, registrar la causa y revisar la estrategia con los picos medidos y la preferencia vigente del usuario. Si éste cambia la restricción, revisar también los wrappers que aún la imponen; no perpetuar el gate anterior ni retirar límites útiles sin motivo. No confundir una cancelación del supervisor con OOM o fallo del producto, ni terminar aplicaciones ajenas para lograr margen. Un límite del propio harness insuficiente para su reserva de direcciones también requiere diagnóstico separado.

## Investigaciones con plazo fijo

Dar al worker una pregunta causal concreta y un entregable que pueda sobrevivir
al corte: candidatos priorizados, fuente/revisión exacta, evidencia a favor y
en contra, y mínima prueba que los falsaría. Guardar ese ledger y checkpoints
durante la investigación; muchas lecturas sin síntesis no son un resultado.
Registrar modelo, esfuerzo, duración autorizada y duración/terminación real.
Reservar parte del plazo para entregar antes del hard stop, sin depender de
que el worker alcance un mensaje final.

Un defecto ajeno al síntoma no cumple el objetivo causal. Conservarlo por
separado y seguir la cadena pertinente y su alternativa, sin terminar sólo por
hallar una pista ni rellenar tiempo cuando ya no queda una prueba útil.
Entregar una conclusión negativa acotada es válido: distinguir causa probada,
hipótesis y evidencia aún necesaria, sin prometer descubrir una causa dentro
del plazo ni presentar recuperación/observabilidad como corrección causal.

## Cierre y worktrees

Al completar una tarea preguntar en el registro **¿el worktree sigue siendo necesario?** Conservarlo si está activo, en review, mantiene una reproducción o contiene artefactos necesarios. Si está integrado y verificado, preservar primero evidencia, WIP y archivos ignorados que hagan falta; después archivar el worktree mediante la herramienta gestionada o Git según el entorno. No archivar mientras otro worker o build lo usa.

Para ordenar ubicaciones, esperar quiescencia y usar herramientas de worktrees/Git, sin mover carpetas activas a mano. En Windows, resolver y verificar rutas absolutas antes de mover o borrar recursivamente: deben quedar dentro del destino autorizado. Una carpeta central como `project-worktrees` puede ser convención del proyecto; registrar allí sólo sus checkouts. Excluir SDKs/cachés compartidos, documentos, assets y la instalación del usuario. Mantener el workspace liviano conservando recuperación útil, sin añadir ceremonias ni borrar trabajo para mejorar apariencia.
