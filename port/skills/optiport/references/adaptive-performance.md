# Rendimiento adaptativo sin regresiones gratuitas

Aplicar cuando el producto deba funcionar en equipos de capacidades distintas. Definir el objetivo de latencia/memoria y las funciones/calidad que deben conservarse. Clasificar perfiles bajo/medio/alto por capacidades y mediciones pertinentes, no sólo RAM, frecuencia o nombre comercial. Reutilizar el pipeline y calidad actuales en equipos capaces; no imponer un cap global sin evidencia de su necesidad.

Si encaja en el producto, ofrecer **Auto / Rendimiento / Calidad** con significado claro. Auto conserva paridad, lógica y resultado; reduce sólo trabajo opcional ante presión observada o un presupuesto explícito. Recuperar calidad al desaparecer presión mediante hysteresis y ventanas estables, evitando oscilación y recreación continua de recursos. Un override no puede ignorar límites reales de seguridad; explicar degradación/fallback con precisión.

En aplicaciones interactivas, reducir frecuencia de presentación o detalle opcional sólo si preserva cálculos, input y sincronización. Saltar presentación no autoriza omitir pasos de procesamiento ni descartar audio incorrectamente. En ingeniería/datos, mantener precisión del resultado autoritativo y export; adaptar preview únicamente donde su contrato lo permita. En diseño, preservar original, color, undo y fidelidad final.

## Presupuestos y límites

Medir CPU/GPU, memoria y coste de recursos con su significado real. Un presupuesto de buffers del proceso no es VRAM disponible ni evidencia de FPS. Si no puede consultarse memoria GPU fiable, registrar esa incertidumbre: usar fallback conservador razonado y, cuando sea viable, override validado; no inventar capacidad. Un límite de seguridad puede seguir siendo necesario incluso en hardware potente, pero debe vincularse al recurso/espacio de direcciones que protege y permitir la calidad compatible.

Auditar caps existentes por efecto: un presupuesto conservador puede reducir supersampling en equipos capaces sin demostrar una ganancia de frame pacing. Mantener ese análisis en el proyecto hasta medirlo; no documentar como garantía de calidad/estabilidad un número heredado de otro proyecto.

## Validación de ambos extremos

Comparar baseline y cambio en perfiles bajo y alto con escenario/calidad controlados: p95/p99 cuando haya muestras suficientes, memoria/picos, calidad observada y estabilidad del flujo. Probar cold/warm, cambio de perfil y pausa/reanudación; cubrir distintos dispositivos disponibles y dejar explícitos los no probados. Comprobar que el equipo capaz conserva funciones/calidad y no empeora, además de que el limitado mejora de forma medible. No prometer cero bugs ni extrapolar un equipo al resto.

Ejemplo: una carga HD agota el pool en un equipo limitado. Medir solapamiento y consumo, reducir recurso opcional o calidad con fallback y recuperar con hysteresis; en el equipo capaz, verificar que sigue usando la calidad completa. No convertir el fallo de uno en un cap fijo para todos ni cambiar el resultado autoritativo para mejorar la presentación.
