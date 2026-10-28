# Investigación selectiva de repositorios

Significa encontrar proyectos relevantes y combinar patrones útiles adaptados a la aplicación; no nombra un repositorio concreto. Usar sólo cuando una carencia o decisión justifique investigar fuera. El diagnóstico, reproducción, implementación y verificación descritos en la skill no requieren conexión ni consultar todo el catálogo.

Para seleccionar lecturas, reutilizar evidencia y decidir cuándo ampliar o parar, aplicar [recuperación escalonada](progressive-retrieval.md). No cargar todo el catálogo ni el historial de cada proyecto por defecto.

1. Definir la función o problema, restricciones del proyecto y criterio medible de éxito. Construir el perfil mínimo: formato/binario, arquitectura, ABI, compilador y opciones probables, SDK/runtime y versión, plataforma/backend y contrato relevante. Para una aplicación web puede importar protocolo y versión de dependencia; para un codec, formato y variantes; para un objeto nativo, calling convention y toolchain. No aplicar una receta de otra plataforma por compartir lenguaje. Estrellas, recomendaciones y foros ayudan a descubrir candidatos, pero no prueban calidad, compatibilidad ni rendimiento.
2. Pasar de la recomendación a fuentes primarias. Leer uno o dos archivos de implementación/API pertinentes, sus pruebas cuando existan y licencia/avisos. Registrar URL fijada a commit, fecha de revisión y señales de mantenimiento; una fecha reciente por sí sola no garantiza soporte. No ejecutar código ajeno ni clonar proyectos enteros para una lectura puntual.
3. Comparar arquitectura, modelo de memoria, threads, backends, formatos, hardware y dependencias con el proyecto actual. Explicar qué mecanismo se rescata y qué supuestos no encajan. El [catálogo](source-catalog.md) ofrece pistas filtradas, no recetas aprobadas para todo motor.
4. Elegir entre adoptar una dependencia, adaptar un patrón o mantener la solución existente. Respetar licencia y atribuciones del material efectivamente reutilizado; MIT de OptiPort no cambia licencias externas ni concede derechos sobre aplicaciones. No copiar ciegamente ni trasladar presupuestos numéricos sin medir.
5. Probar en una reproducción aislada, comparar con baseline y verificar regresiones/paquete según el alcance. Promover a lección sólo mecanismos demostrados; guardar ideas prometedoras sin verificar en el roadmap del proyecto con prueba pendiente, sin convertirlas en reglas generales.

Comunicar al usuario sin exigirle conocimientos de código: problema, patrón elegido, mejora medida y límites. Si no se puede consultar una fuente ahora, usar evidencia local suficiente o marcar la idea como pendiente; no inventar lo que hace el repositorio.

## Búsqueda por mecanismo y revisión

Buscar símbolos distintivos, fragmentos de código, constantes combinadas, estructura del algoritmo y nombres anteriores; un homónimo aislado no basta. Si el símbolo no aparece, comprobar nombres mangled/exportados, aliases, funciones internas y variantes de versión. Seleccionar fuentes según el perfil, incluyendo proyectos pequeños y el upstream del SDK/runtime, no sólo los resultados más visibles.

Desde un candidato útil, recorrer sólo las ramas del grafo que puedan resolver la diferencia: fork y upstream, commit que introdujo la función, diff de corrección, versión anterior y configuración de build. Un fork puede conservar una revisión antigua compatible donde el SDK actual difiere. Leer cuerpos completos, headers/layout y opciones de compilación de la revisión fijada antes de adaptar; snippets sirven para descubrir, no para validar. Guardar negativos y variantes descartadas con causa.

Agrupar cuerpos idénticos por hash normalizado de comentarios/espacios para evitar evaluar muchas copias del mismo candidato. Ese hash **no prueba equivalencia binaria**. Conservar identidad de repositorio, commit y licencia de cada fuente usada; no deducir permisos de una API `license:null` ni de la licencia de un fork. Al encontrar fuentes originales de una biblioteca, revisar primero si sus objetos ya están enlazados: reproducirlos puede aclarar procedencia sin añadir código recuperado.

Un 403 puede ser autenticación, permisos o rate limit; revisar respuesta/headers y usar el conector o acceso autenticado disponible. No concluir que un repositorio o archivo no existe. Registrar consultas efectivas y límites de cobertura; una búsqueda profunda no demuestra exhaustividad de GitHub.

Para candidatos binarios o SDK, continuar con [contratos de decompilación](decompilation-contracts.md). Para una biblioteca ordinaria bastan sus contratos de API, formato, concurrencia y pruebas pertinentes: no exigir identidad binaria a una migración funcional que no la requiere.

## Verificar afirmaciones de completitud

Antes de usar «100%» como señal, fijar commit, versión/región y denominador: bytes, funciones, unidades enlazadas, C recuperado, datos medidos o nombres. Consultar README y generador del reporte; cuando exista, descargar el artefacto CI asociado al mismo SHA y revisar sus cifras/exclusiones y el job que valida el producto. Un workflow en verde puede verificar sólo código con assets sustituidos, conservar ASM original o medir datos totales cero. No convertir eso en «todo el programa recuperado en C» ni «todos los datos entendidos».

Comprobar si la fuente completa permite reconstruir con entradas propias y toolchain documentada, separando lo publicado, lo verificado por CI y lo reproducido localmente. Porcentajes de forks, regiones o fechas diferentes no se intercambian. El resultado útil es fuente concreta compatible y prueba viable, no una clasificación por estrellas.
