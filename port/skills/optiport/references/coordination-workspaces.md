# Coordinación y workspace liviano

Aplicar cuando haya trabajo paralelo autorizado o worktrees que gestionar. Reutilizar la autoridad de pendientes existente; no crear una segunda cola para repetirla. Un snapshot manual informa, pero no sustituye una cola con ownership actualizado. Roles y herramientas concretos pertenecen a la documentación del proyecto, no a una regla universal de OptiPort.

## Claim, evidencia e integración

Antes de editar, comprobar la autoridad y reclamar la tarea mediante su mecanismo atómico/CAS (compare-and-swap) si existe. Registrar dueño, alcance/archivos, dependencias y branch; si otro claim ganó, replanificar sin pisarlo. Sin CAS disponible, usar coordinación explícita y evitar solapamiento, sin fingir exclusión atómica. Un lock de integración serializa al integrador; no concede propiedad sobre el trabajo ajeno.

Mantener en la tarea evidencia compacta: SHA fuente probado, pruebas y resultado, PR, merge, artefacto/versión cuando aplique y próxima prueba mínima. Separar estos estados: editar, compilar, revisar, integrar y entregar no son equivalentes. Revisar de forma independiente lo que el riesgo requiera. Tras integrar, probar la combinación final con su SHA; compilar un fixture o fuente previa no valida el resultado combinado.

Si un rebase/merge falla, detener pruebas y pushes que dependan de esa operación hasta resolverlo y confirmar el estado final. No continuar sobre una premisa de actualización fallida. Cachés bare por worker pueden reducir transferencias/historial repetido, pero el claim/CAS debe refrescar la autoridad antes de decidir ownership; una caché desactualizada no decide quién puede editar.

Delegar sólo dentro de la autorización: pregunta acotada, contexto mínimo, restricciones, archivos/dependencias y evidencia esperada; conservar modelo/esfuerzo autorizados. Compartir resultados dentro del trabajo no autoriza comunicaciones externas, comentarios ni cierres de issues.

## Cierre y worktrees

Al completar una tarea preguntar en el registro **¿el worktree sigue siendo necesario?** Conservarlo si está activo, en review, mantiene una reproducción o contiene artefactos necesarios. Si está integrado y verificado, preservar primero evidencia, WIP y archivos ignorados que hagan falta; después archivar el worktree mediante la herramienta gestionada o Git según el entorno. No archivar mientras otro worker o build lo usa.

Para ordenar ubicaciones, esperar quiescencia y usar herramientas de worktrees/Git, sin mover carpetas activas a mano. En Windows, resolver y verificar rutas absolutas antes de mover o borrar recursivamente: deben quedar dentro del destino autorizado. Una carpeta central como `project-worktrees` puede ser convención del proyecto; registrar allí sólo sus checkouts. Excluir SDKs/cachés compartidos, documentos, assets y la instalación del usuario. Mantener el workspace liviano conservando recuperación útil, sin añadir ceremonias ni borrar trabajo para mejorar apariencia.
