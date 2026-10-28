# Evidencia y casos acotados del módulo

Leer al revisar o ampliar [contratos de decompilación](decompilation-contracts.md), no en cada aplicación de OptiPort. Este registro explica qué decisiones sustentan las lecciones; no acredita mejoras universales de velocidad, equivalencia de binarios arbitrarios ni calidad por popularidad.

## Evidencia de partida

- Comparador: el [historial público de verificación](https://github.com/MatyMeatBoy/SS2Decomp/blob/8853ac3b62ebd2d6efe4b1c092292b7456fae2cb/docs/agents/MATCH_VERIFICATION.md) documenta falsos positivos al ordenar instrucciones y borrar addends/registros de relocaciones, y fixtures negativos que los detectan. Para aplicar la lección, fijar revisión del verificador del proyecto actual y repetir sus pruebas; este historial no certifica cualquier comparador posterior.
- SDK: [fuente histórica del payload](https://github.com/pret/colosseum-mb/blob/65ef452784d15091028c202b85fe05bdae0ae690/payload/src/m4a.c) frente a [otra revisión del driver](https://github.com/pret/pokefirered/blob/ce6406d8c2eaf46d543c8a2606de0c3cb1951982/src/m4a.c), con [integración pública verificada](https://github.com/MatyMeatBoy/SS2Decomp/commit/8853ac3b62ebd2d6efe4b1c092292b7456fae2cb). La investigación comparó cuerpos completos, agrupó copias idénticas y compiló la variante en contexto del módulo antes de reconstruir el artefacto integral. No se promueve ninguna hipótesis de inicialización que no pasó esa comparación.
- Runtime: [Makefile y fuentes de libgcc fijados](https://github.com/pret/agbcc/tree/da598c1d918402c42c0c0d7128ba14567f3175e9/libgcc) permiten reproducir miembros del archive con su frontend/opciones, diferenciando helpers C de veneers ASM. Que ya estén enlazados cambia la cola de trabajo, no el número de C recuperado.
- Completitud: [reporte CI de una reconstrucción nativa](https://github.com/overjt/knidl/actions/runs/37331601690), [reporte de otra versión/proyecto](https://github.com/testyourmine/kleod/actions/runs/39687164516) y sus README/generadores muestran por qué code, datos, ASM excluido y funciones tienen denominadores distintos. Consultar sus artefactos y SHA, no transferir porcentajes a otro proyecto ni conservar cifras efímeras en el método.
- Evaluación de instrucciones: cinco respuestas control y cinco guiadas sobre el mismo escenario simulado mostraron omisiones de clasificación apphost/bundle/managed, identidad módulo+RVA y revisión ABI en controles; algunas respuestas ya mantenían clasificación provisional correcta. Las guiadas formularon esas acciones antes de elegir herramientas. Fue evaluación de decisiones redactadas, **no** cinco reconstrucciones reales ni benchmark de rendimiento. Datos y resultados de esa evaluación permanecen en el proyecto que la ejecutó.

## Casos de aplicación independientes

Estos son controles de decisión revisados contra las instrucciones, no ejecuciones de los productos ni una evaluación ciega adicional. Al cambiar el método, comprobar que la elección y el límite se mantienen:

| Caso | Decisión esperada | Rechazo que distingue el caso |
|---|---|---|
| PE x64 launcher sin CLR visible, DLL acompañante y código restante desconocido | Inspeccionar componentes y metadatos/bundle; identidad módulo+RVA; fijar ABI Windows y alcance antes de herramienta | No declarar nativo puro ni porcentaje global desde tamaño EXE o catálogo parcial |
| Plugin de diseño con codec homónimo encontrado en un fork de otra arquitectura | Comparar formato/versiones/layout/ABI y licencia, leer fuente completa y probar entradas válidas/malformadas con oráculo | No copiar por nombre ni exigir bytes idénticos si el objetivo es compatibilidad funcional |
| Runtime de firmware reconocido en archive ya enlazado | Reproducir miembro correcto con flags reales y revisar relocaciones/datos asociados si cambia una decisión | No contar como C nuevo ni volver obligatorio convertir ASM original retenido |
| Servicio web con SDK HTTP versionado y regresión de paginación | Perfilar protocolo/API, versión, errores y concurrencia; buscar diff/historial y verificar el recorrido | No importar recetas de registers, linker, m2c o ABI nativa a una falla de protocolo |
| Dos regiones de una función gigante y varios workers | Propietario único, subclaims previos o análisis separado, checkpoints y validación integral en integración | No editar mismo cuerpo/base concurrentemente ni sumar regiones como funciones terminadas |
| Tracker externo dice100%, CI tiene total_data0 y objetos ASM conservados | Inspeccionar commit, generador/denominadores y artefacto; reportar sólo dimensión medida | No afirmar todos los datos decodificados ni todo C por un badge |

Para una evaluación futura con agentes independientes, conservar prompts, condiciones, respuestas, criterio y fallos observados antes/después; usar modelo/esfuerzo y recursos autorizados. No presentar esta revisión de casos como ese experimento. Un cambio documental no mide aceleración de software.
