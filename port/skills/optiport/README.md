# OptiPort

**AI skill for incremental debugging and optimization of software, source ports, recompilations, decompilations.**

Una skill para **mejorar la estabilidad y el rendimiento del software** con evidencia reproducible, aunque quien la use no programe.

Se aplica a aplicaciones web, escritorio, ingeniería, diseño y procesamiento de datos. Conserva precisión, modelos, edición y fidelidad de exportación según el dominio.

No se queda en un informe: investiga, implementa mejoras, las prueba y sube bloques revisables cuando está autorizado. Explica qué cambió, qué se midió y qué falta comprobar.

## Presentación profesional

Presentar y aplicar OptiPort exclusivamente como método profesional de optimización de software general. Todos los ejemplos, informes, títulos y explicaciones deben referirse al producto, sus flujos y sus resultados. Mantener esta regla en futuras ampliaciones.

## Uso

Coloca este repositorio como carpeta `optiport` en el directorio de skills de tu asistente —por ejemplo, `~/.codex/skills/optiport` o `~/.claude/skills/optiport`—. Necesita acceso al código y herramientas de compilación del proyecto.

Pídele:

> Usa OptiPort para revisar y mejorar esta aplicación. Prioriza cierres, latencia y consumo de memoria, conserva mis documentos e instalación principal, implementa lo que puedas verificar y sube los cambios por bloques. Explícame el avance sin tecnicismos.

## Qué revisa

- Memoria, alineación y compatibilidad entre arquitecturas.
- Migración de 32 a 64 bits con compatibilidad de datos, documentos y funciones; comparar antes de recomendarla, porque 64 bits no garantiza más velocidad.
- Fallbacks que conservan el software original cuando falla un recurso opcional.
- Tiempos de carga, memoria y latencia de operaciones y regularidad de presentación mediante profiling.
- Cachés, presupuestos, carga asíncrona, precarga y streaming.
- Audio, vídeo, texturas, colas acotadas y trabajo sin progreso.
- Suspensión, reanudación, limpieza de recursos y protección del guardado.
- Logs rotativos, regresiones, pruebas prolongadas y builds verificables.
- Organización del proyecto comprensible para personas, worktrees reunidos, releases y pruebas identificables, almacenamiento sin duplicaciones innecesarias y VM dimensionadas según su trabajo real. La [guía de organización y almacenamiento](references/workspace-storage.md) incluye ayudas visuales opcionales para carpetas y controles propios de ingeniería.

Sirve como método para cualquier plataforma; las pruebas disponibles dependen del proyecto, herramientas y dispositivos. No promete arreglar todos los fallos ni inventa porcentajes de mejora. Publicar releases o enviar builds requiere autorización.

[Instrucciones para la IA](SKILL.md) · [Capas de verificación](references/verification-layers.md)

Cada entrega aplica [controles de regresión y preparación](references/release-readiness.md): evidencia del paquete exacto, fallos conocidos y límites por plataforma. La IA elige las comprobaciones pertinentes y explica lo pendiente sin convertirlas en tareas técnicas para quien usa la skill.

Para migraciones, consulta la [guía de arquitectura](references/architecture-migration.md). El objetivo puede ser un paquete Windows o Linux con ambos ejecutables y selección compatible; en Android, un APK con las ABI realmente soportadas. La disponibilidad depende del código, toolchains y pruebas de cada proyecto.

Irá creciendo con aprendizajes comprobados. Para proponer uno, aporta el fallo, reproducción o evidencia, solución y límites; evita subir archivos protegidos, assets comerciales, documentos o datos privados.

## Comparar antes y después

Usar el mismo conjunto de datos, operación, calidad y configuración; registrar versiones, equipo, repeticiones y condiciones de caché fría/caliente. Comparar latencia, memoria y equivalencia del resultado. Si falta una medición comparable, describir el cambio como implementado y pendiente de medición; no afirmar una mejora. La [guía de software general](references/software-general.md) incluye un registro mínimo reutilizable.

## Próximos aprendizajes

Pendientes del método: incorporar ejemplos reproducibles de pruebas prolongadas, matrices de dispositivos y cancelación de resultados de una escena anterior al reiniciar recursos. Cada ejemplo deberá indicar qué probó el harness y qué se ejecutó en un dispositivo real.

Estos pendientes no certifican resultados de ningún software. Los recorridos prolongados, la matriz de hardware y los resets reales de dispositivo siguen requiriendo evidencia propia de cada proyecto; sus tickets y resultados se mantienen en el proyecto correspondiente.

## Licencia

El material original de esta skill se publica bajo [MIT](LICENSE). Esta licencia no concede derechos sobre código, archivos protegidos, fuentes, assets ni otros materiales de las aplicaciones analizadas.
