# Investigación selectiva de repositorios

Significa encontrar proyectos relevantes y combinar patrones útiles adaptados a la aplicación; no nombra un repositorio concreto. Usar sólo cuando una carencia o decisión justifique investigar fuera. El diagnóstico, reproducción, implementación y verificación descritos en la skill no requieren conexión ni consultar todo el catálogo.

Para seleccionar lecturas, reutilizar evidencia y decidir cuándo ampliar o parar, aplicar [recuperación escalonada](progressive-retrieval.md). No cargar todo el catálogo ni el historial de cada proyecto por defecto.

1. Definir la función o problema, restricciones del proyecto y criterio medible de éxito. Buscar candidatos por área; estrellas, recomendaciones y foros ayudan a descubrirlos, pero no prueban calidad, compatibilidad ni rendimiento.
2. Pasar de la recomendación a fuentes primarias. Leer uno o dos archivos de implementación/API pertinentes, sus pruebas cuando existan y licencia/avisos. Registrar URL fijada a commit, fecha de revisión y señales de mantenimiento; una fecha reciente por sí sola no garantiza soporte. No ejecutar código ajeno ni clonar proyectos enteros para una lectura puntual.
3. Comparar arquitectura, modelo de memoria, threads, backends, formatos, hardware y dependencias con el proyecto actual. Explicar qué mecanismo se rescata y qué supuestos no encajan. El [catálogo](source-catalog.md) ofrece pistas filtradas, no recetas aprobadas para todo motor.
4. Elegir entre adoptar una dependencia, adaptar un patrón o mantener la solución existente. Respetar licencia y atribuciones del material efectivamente reutilizado; MIT de OptiPort no cambia licencias externas ni concede derechos sobre aplicaciones. No copiar ciegamente ni trasladar presupuestos numéricos sin medir.
5. Probar en una reproducción aislada, comparar con baseline y verificar regresiones/paquete según el alcance. Promover a lección sólo mecanismos demostrados; guardar ideas prometedoras sin verificar en el roadmap del proyecto con prueba pendiente, sin convertirlas en reglas generales.

Comunicar al usuario sin exigirle conocimientos de código: problema, patrón elegido, mejora medida y límites. Si no se puede consultar una fuente ahora, usar evidencia local suficiente o marcar la idea como pendiente; no inventar lo que hace el repositorio.
