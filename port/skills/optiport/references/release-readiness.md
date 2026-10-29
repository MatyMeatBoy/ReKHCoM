# Preparación de cada entrega

Aplicar al preparar un build, paquete o release, ajustando evidencia al riesgo, rutas modificadas, mecanismo del fallo y alcance anunciado. Una prerelease limitada autorizada puede declarar áreas fuera de alcance como no verificadas; una estable requiere evidencia más amplia de compatibilidad y regresión para el soporte que promete. Reutilizar board y pruebas existentes, sin imponer una lista idéntica a cada entrega ni pedir permisos adicionales por esta guía.

Elegir escenarios breves y automatizables por patrón de riesgo: transición, acceso límite, recurso fallido, cambio de arquitectura o recuperación de estado. Buscar primero la hipótesis decisiva de menor coste en logs, símbolos del binario, código afectado e historia relevante; consultar repos externos o ampliar corpus sólo si falta evidencia concreta. No exigir un recorrido completo de principio a fin como gate universal; reservar sesiones prolongadas para riesgos de acumulación.

Convertir cada crash de usuario reproducido en una regresión estable con nombre, escenario mínimo y postcondición observable; `exit 0` solo no prueba que el flujo se completó. Registrar matriz de arquitecturas/configuraciones afectadas y procedencia inmutable de fixtures privados, con sustituto público saneado si puede construirse y redistribuirse. Conservar la regresión en entregas futuras, incluidas migraciones de arquitectura, y ejecutarla sobre el artefacto empaquetado exacto tras reconstruirlo. Ejemplo: cargar la misma copia del estado persistido y abrir la siguiente vista debe alcanzar el estado esperado, no sólo evitar la excepción.

## Evidencia mínima para decidir

Registrar por control **pass**, **fail**, **unverified** o **N/A**, con evidencia y motivo. `N/A` necesita una razón de alcance; falta de hardware es `unverified`. Un harness, un arranque o una plataforma aprobada no certifican las demás.

Si hay reportes relacionados, cierres o reaperturas, contrastar la [cadena de issue a artefacto](progressive-retrieval.md#relacionar-reportes-y-entregas). Un fix integrado no demuestra que esté publicado o presente en el paquete del usuario.

| Control pertinente | Evidencia esperada |
| --- | --- |
| Regresión del fallo | Vincular bug/issue ID, reproducción original, resultado y flujo relacionado. Derivar las plataformas afectadas del código compartido y backends: un fallo reportado en Linux también puede afectar Windows. Probar el recorrido correspondiente donde sea viable. |
| Arquitecturas y progreso | Comparar funciones 32/64, incluido HD; probar interoperabilidad de documentos y configuración con copias. Para snapshots, verificar compatibilidad o rechazo claro y seguro cuando no sean portables. No convertir un documento válido para un build en una promesa universal. |
| Ciclo de vida y recursos | Según las rutas tocadas: foco, pausa/reanudación, assets HD válidos y fallidos, presupuestos, audio/vídeo, memoria y sesiones prolongadas. Observar limpieza, crecimiento y estado al volver al software. |
| Paquete final | Registrar SHA exacto de fuentes, configuración/toolchain y hash del artefacto. Probar instalación aislada, arranque y flujo afectado usando el paquete que se entregará, con sus bibliotecas y assets permitidos. Un ejecutable suelto no valida el archivo distribuido. |
| Vigencia | Asociar resultados a SHA, arquitectura, opciones, assets y paquete probados. Tras cambios relevantes, repetir controles afectados; justificar evidencia previa que siga siendo aplicable. Identificar evidencia obsoleta en lugar de arrastrar un pass. |

Un repack de launcher, scripts o metadatos puede conservar binarios ya verificados: registrar por separado la revisión que los compiló y la del empaquetado, comprobar equivalencia de los inputs pertinentes y hashes de los miembros reutilizados, y probar el punto de entrada modificado desde el paquete final. No atribuir esos binarios al HEAD nuevo ni exigir recompilar lo que no cambió. Una fecha de copia posterior al commit no acredita compilación ni invalida bytes idénticos. Tras un squash, contrastar los blobs/inputs relevantes con el merge final: la ausencia de ascendencia del SHA de la rama no demuestra una diferencia de código.

Si una prueba dedicada y la entrega oficial usan productores distintos, comprobar que ambos incorporan el cambio: compartir bibliotecas nativas no demuestra igualdad del arranque, manifiesto, permisos o configuración que las rodea. Inspeccionar el manifiesto **compilado dentro del paquete final** y la resolución real de su punto de entrada, no sólo el XML/plantilla fuente o el resultado del builder de prueba. Por ejemplo, un APK puede conservar las mismas bibliotecas y seguir entrando por la Activity anterior. Verificar instalación y apertura por ese punto de entrada donde sea viable; una inspección estática correcta no acredita el runtime del dispositivo.

## Código, contenido importado y calidad efectiva

Un binario reciente y sus recetas actualizadas identifican al productor, pero no actualizan por sí solos una importación anterior. Registrar por separado la identidad del binario, revisión del importador/recetas, manifiesto del contenido realmente usado y captura/log del consumidor, con perfil, región y modo pertinentes. Un interruptor HD activado o las recetas presentes dentro del ejecutable no demuestran que ese cuadro empleó los recursos nuevos.

Comparar el conjunto **requerido por las recetas actuales para el alcance probado** con lo importado: incluir familias y variantes nuevas, ausencias, rechazos y generación/formato del contenido. Que todos los archivos antiguos conserven su hash sólo verifica esos archivos. Si faltan recursos nuevos, comprobar que la actualización/reimportación los detecta y produce, usando entradas propias en un árbol privado; no sustituir silenciosamente el contenido aprobado ni distribuir entradas protegidas.

Clasificar la ejecución como contenido completo, parcial, fallback nativo o importación heredada, según lo realmente cargado y dibujado. Una comparación completa/parcial debe cambiar únicamente los recursos bajo prueba y acreditar la calidad aplicada mediante el consumidor real. Un fallback correcto puede aprobar su control específico, pero no una promesa de presentación HD completa ni una mejora de rendimiento con calidad equivalente.

## Cerrar el artefacto antes de entregarlo

Mantener el staging mutable identificado como candidato. Antes de poner una ruta o enlace a disposición del usuario o del coordinador que la entregará, terminar sus escrituras y registrar estado listo, versión/nombre y hash. La existencia del archivo o un hash tomado mientras otro worker escribe no constituyen ese cierre; usar la autoridad de coordinación existente para distinguir candidato de entregado.

Desde la primera entrega, incluida una ruta local antes de publicar una release, conservar esos bytes e identidad como inmutables. No regenerar ni sobrescribir el mismo archivo entregado para cambiar notas, manifiesto o código, aunque se conserve su nombre o la diferencia parezca sólo documental. Aportar notas/validaciones posteriores como suplementos nuevos vinculados al hash original; si cambian los bytes del paquete, crear otra identidad de archivo o versión y verificarla por separado. Mantener el artefacto anterior y la evidencia que lo valida.

Separar candidato local cerrado, assets subidos a draft y release pública. Antes de publicar, conciliar el destino del tag con la revisión final y comprobar nombres, tamaños y digests remotos frente al inventario cerrado; para una descarga automática, ejercitar el matcher y filtro reales con todos los assets, incluidos los auxiliares. Después comprobar tag, visibilidad, canal estable/prerelease, Latest cuando corresponda y mismos assets. Un digest remoto prueba identidad, no disponibilidad para el reportero ni presencia de un fix sin procedencia; un enlace de draft no sustituye una entrega accesible. Estas comprobaciones no amplían la autorización para publicar o enviar mensajes.

## Decisión y comunicación

Un fallo reproducible conocido bloquea declarar lista la entrega con ese alcance. Corregirlo y repetir la regresión o reducir explícitamente el alcance de una prerelease limitada ya autorizada; no presentar el fallo como resuelto. La evidencia pendiente impide afirmar verificación de esa plataforma o función. No prometer ausencia de bugs ni «arreglado en todas las plataformas» sin evidencia correspondiente.

Un fallback intencional puede conservar software y progreso reduciendo calidad; registrar su causa y qué función/calidad se pierde. No ocultar pérdida de HD, audio, vídeo u otra función bajo una afirmación de paridad o estabilidad. Una prerelease limitada debe nombrar plataformas, arquitecturas, funciones y comprobaciones pendientes para que el usuario pueda decidir con información concreta.
