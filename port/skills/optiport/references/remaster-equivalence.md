# Equivalencias entre original y remake/remaster

Aplicar cuando un port conserve el motor original y adapte recursos o presentación
de otra edición. Identificar edición/región y evidencia de cada referencia. Un
atlas, una receta de extracción o un documento local no prueban por sí solos la
fórmula del ejecutable del remake. Marcar las aproximaciones pendientes de contraste.

Separar tres contratos: estado autoritativo del original, transformación a la
presentación nueva y comportamiento del fallback. No cambiar daño, vida, reglas,
timing o guardados para conseguir que una barra parezca más larga. Si el usuario
pide cambiar reglas, tratarlo como una función distinta y explícita.

## Matriz de correspondencias

Para el elemento modificado, registrar fuente del dato, unidades/rango, curva de
conversión, límites, transiciones y referencia visual. Evitar una tabla exhaustiva
del juego: empezar por el defecto reportado y ampliar a consumidores hermanos.

| Elemento | Precisiones que cambian la implementación |
| --- | --- |
| HP/MP/medidores | Distinguir valor actual, máximo autoritativo, valor interpolado, daño pendiente, capacidad de la pista y número de capas. Una barra llena puede ocupar sólo parte de un marco si la longitud representa capacidad absoluta. No confundir ese diseño con daño ni asumir que una razón normalizada es equivalente. |
| Jugador/enemigo/jefe | Confirmar escalas distintas, capas superiores y cambios de objetivo. Vincular estado visual a identidad y generación del actor, sin trasladar interpolaciones del objetivo anterior. |
| Layouts animados | Identificar cuadros de entrada/salida, controles de longitud y animaciones cíclicas. Un grupo que codifica capacidad debe mantenerse en el cuadro correspondiente; pasarle un reloj creciente puede hacer crecer y reiniciar la pista con HP constante. |
| Cartas, iconos y marcos | Distinguir ilustración reemplazada de borde/corona/fondo conservados. Si la receta HD sólo cubre una ventana, componer el original necesario debajo; no ocultar todo el objeto por encontrar una textura HD. |
| Texto/prompts | Mantener significado, acción efectiva y localización. Compartir apariencia con el remake no autoriza cambiar la convención de botones o tutoriales sin mapearlos. |

## Pruebas por invariantes

Tomar valores conocidos del estado real y probar cero, uno, intermedios, lleno,
límites de capa y máximos superiores al diseño habitual. Con actor, HP y máximo
constantes, la capacidad visible debe permanecer estable durante varios ciclos
de animación. Comprobar monotonía, saturación, daño, curación, cambio de objetivo,
entrada/salida y reset; separar transición deliberada de deriva acumulada.

Comparar original, HD listo y fallback con el mismo estado y encuadre. Afirmar
equivalencia sólo sobre lo medido; conservar diferencias deliberadas documentadas.
Una captura aislada no demuestra la curva ni estabilidad temporal. Registrar
valores y varios cuadros junto a la imagen para distinguir capacidad de animación.

Para bordes, probar escalas/resoluciones representativas y ruta GPU real. Medir
la caja de trabajo de antialiasing: un medidor corto no justifica procesar siempre
un lienzo grande. Recortar o reutilizar trabajo cuando sea correcto antes de subir
presupuestos globales. Verificar alpha premultiplicado, filtrado, seams y capas
interiores; una prueba headless no certifica suavidad visual. Mantener fallback
seguro y registrar cuándo pierde suavizado o fidelidad.
