# Recuperación escalonada de evidencia

Aprender lo necesario para una decisión concreta y conservar evidencia reutilizable, sin sacrificar calidad por ahorrar tokens. Ajustar el presupuesto de lectura a incertidumbre y riesgo; no imponer una cuota universal de búsquedas.

## Buscar y ampliar

Empezar por logs locales, reproducción, símbolos del binario, código afectado y headers de la versión instalada. Buscar error, función, arquitectura o transición; leer rangos de función y callers pertinentes en lugar de archivos, repositorios o documentación completos. Perfilar el escenario antes de atribuir tirones a una optimización supuesta.

Si falta una explicación local, consultar metadatos filtrados por términos de error/función, arquitectura, labels y versión. Distinguir issues abiertos/cerrados y PR abiertos/cerrados/merged. Leer después uno o dos comentarios que aporten reproducción o resolución, el diff relevante y su prueba; ampliar sólo si no bastan. No descargar historiales completos por defecto.

Un issue cerrado no demuestra arreglo; un PR cerrado no demuestra merge. Si se atribuye una corrección al código actual, comprobar que su merge SHA está presente en la rama objetivo actual. Un caso cerrado parecido aporta una hipótesis, no prueba que sea el mismo bug. Estrellas y foros descubren candidatos; contrastar el mecanismo con código primario y versión concreta.

## Relacionar reportes y entregas

Reconstruir sólo el tramo necesario: **issue → reproducción → PR del fix → merge SHA → presencia en rama objetivo → SHA/versión del artefacto → regresión ejecutada → confirmación del reportero**. Merged, publicado y presente en el build del usuario son hechos distintos. Un cierre como `not planned`, duplicado o sin reproducción no equivale a corregido.

Consultar primero metadatos y referencias entrantes/salientes: otros issues abiertos que mencionen el cerrado o el PR, backlinks, duplicados y reaperturas. Filtrar por términos, plataformas y versiones; leer sólo enlaces que puedan cambiar la decisión. No recuperar el grafo completo ni asumir que un duplicado, reapertura o mezcla de plataformas comparte causa.

Comparar reproducción, stack y configuración para decidir: artefacto antiguo que necesita actualizarse; corrección ausente que requiere backport; nueva causa con síntoma parecido; o regresión de una corrección presente. Por ejemplo, una investigación puede pasar de alineación a paleta aun manteniendo el síntoma visual: conservar ambas hipótesis hasta que evidencia las distinga, sin convertir el caso de una aplicación en regla general.

Priorizar corrupción de documentos y crashes por impacto antes que cosmética o limpieza de rendimiento. Mantener pendientes compactos con estado, confianza sustentada por evidencia y próxima prueba mínima; la confianza no sustituye una prueba. Un fallo conocido reproducible dentro del alcance anunciado sigue bloqueando esa entrega. No cerrar ni comentar issues automáticamente: estas acciones requieren autorización de comunicación.

### Casos de decisión

- **Fix merged, usuario con paquete antiguo:** el merge está en target, pero el artefacto reportado es anterior. Verificar el paquete que contiene el fix y repetir el recorrido; recomendar actualización cuando esté disponible. No escribir un parche duplicado ni declarar confirmación del reportero sin ella.
- **Issue reabierto con stack diferente:** el artefacto sí contiene el merge, pero la nueva traza apunta a otra ruta. Reproducir esa ruta y contrastar nueva causa/regresión; no descartar el reporte por el cierre previo ni revertir el fix sin evidencia.
- **Issue cerrado y otro abierto lo menciona desde Windows:** el original era de Linux y se cerró sin plan. Leer motivo, reproducción y código compartido, comprobar versiones y probar el backend afectado. No atribuir un fix inexistente ni extrapolar éxito entre plataformas; registrar Windows como pendiente si no hay prueba.

Ampliar la lectura ante evidencia contradictoria, reproducción que no concuerda, fallo de la validación mínima o una frontera ABI desconocida. Una búsqueda sin resultado no prueba ausencia. Consultar el siguiente caller, backend, versión o fuente que resuelva esa incertidumbre específica.

Para reuso de SDK/runtime, ampliar por [perfil de software y revisión](repository-research.md#búsqueda-por-mecanismo-y-revisión): símbolos/fragmentos → implementación completa y layout → fork/historial pertinente → toolchain/ABI → reproducción mínima. Si varias fuentes tienen el mismo cuerpo, agruparlas para evitar repetir la misma prueba; si una biblioteca ya está enlazada, registrar procedencia sin inventar trabajo pendiente. Una tasa límite o falta de autenticación no es evidencia de ausencia. Cuando el objetivo sea matching, leer [sus contratos](decompilation-contracts.md) antes de interpretar scores o progreso ajeno.

## Reutilizar y parar

Guardar en el board o caché local un registro compacto: pregunta, conclusión/hipótesis, repositorio y commit, versión de dependencia/toolchain relevante, fecha, archivo/rango o URL, prueba y límites. Usar hashes o identificadores necesarios; no guardar PII, secretos ni documentos privados. Reutilizar lecturas con fingerprint sin cambios en vez de releerlas; invalidar la parte afectada cuando cambien fuente, versión, configuración, assets o binario, y volver a comprobar lo que dependa de ella.

Parar la exploración cuando haya una hipótesis concreta con evidencia suficiente y una validación mínima que pueda refutarla. Ejecutar esa validación y pasar a implementar, probar y entregar por bloques dentro de la autorización. Si falla, volver a la incertidumbre observada; no seguir navegando por inercia ni presentar una hipótesis como resultado. Resumir hallazgos como evidencia, decisión, prueba y pendiente, sin dumps de fuentes ni cifras de benchmark inventadas.

Cuando la delegación esté autorizada y aporte trabajo independiente, enviar una pregunta acotada, rutas/SHA, restricciones y resultado esperado. Preferir contexto mínimo (`fork_turns="none"` cuando la herramienta lo admita) y salidas concisas con evidencia y límites; mantener el modelo/esfuerzo autorizado. No crear workers sólo para releer evidencia ya válida.
