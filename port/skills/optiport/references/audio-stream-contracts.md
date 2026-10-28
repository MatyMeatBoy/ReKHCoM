# Contratos de audio entre productor y consumidor

Usar cuando un backend reproduce audio a otra velocidad, informa colas
incoherentes o produce tirones durante cargas y transiciones. Identificar el
productor de PCM, el mezclador opcional y el consumidor que entrega al dispositivo.

## Separar frecuencias y unidades

| Dato | Contrato que comprobar |
| --- | --- |
| Frecuencia actual del PCM de entrada | Puede cambiar después de abrir el stream. Un mezclador debe avanzar su pista según esta frecuencia, no según el valor inicial de apertura. |
| Frecuencia del dispositivo/salida | Es el dominio del consumidor y puede ser distinta de la entrada. No identifica por sí sola el tiempo representado por un bloque entrante. |
| Ratio de reproducción | Puede combinar conversión de frecuencia y corrección PLL de la cola. Transportar explícitamente la frecuencia de entrada cuando el mezclador la necesita; inferirla del ratio mezcla ambos mecanismos. |
| Cantidad en cola | Precisar bytes de entrada, frames de entrada o frames del dispositivo. Un frame contiene todas sus muestras de canal. Convertir sólo según el contrato del backend correspondiente. |

Por ejemplo, un segundo de entrada de 64000 frames mezclado con una pista
de 48000 Hz debe consumir aproximadamente 48000 frames de esa pista, aunque el
dispositivo se abriera a 48000 Hz y el ratio incluya una pequeña corrección PLL.
Contar los frames decodificados distingue un error de velocidad de un underrun;
un volumen correcto o una cola positiva no lo hacen.

En un puente con varios hosts, negociar capacidades de pausa/reanudación y las
unidades de la cola por separado de una notificación de frecuencia de entrada.
Un comando opcional nuevo puede servir a un host que conserva un backend de
anillo sin cambiar sus unidades. Un binario antiguo puede ignorarlo: la mejora
requiere un host y guest que implementen ese contrato. Conservar comandos
existentes y comprobar qué devuelve el host
ante una operación desconocida; actualizar también listas de staging de headers.
Probar hosts y guests nuevos/antiguos sin asumir que comparten capacidades.

## Continuidad ante falta de PCM

Primero llenar el presupuesto de arranque y después reanudar el dispositivo.
Durante una pausa intencional, conservar posición y no confundirla con EOF o
fallo del productor. Limpiar el estado de recuperación al resetear/cambiar de
stream para que una cola nueva no reproduzca la cola o el último sample anterior.

Si un consumidor necesita entregar PCM mientras el productor se retrasa, una
rampa breve y acotada desde la última muestra hacia silencio, seguida de una
entrada progresiva al recuperar PCM, puede evitar discontinuidades bruscas.
Dimensionar duración y scratch por frecuencia/formato; acotar el trabajo ante
peticiones grandes. No añadir I/O, decodificación, trabajo del juego ni
asignaciones propias al callback; documentar si la API de encolado puede asignar
internamente en vez de afirmar que toda la ruta está libre de asignaciones.
La recuperación sintetiza salida: no repite un paquete antiguo ni avanza el
reloj, decoder o estado autoritativo para simular que llegó contenido nuevo.
Si la entrega al consumidor falla, descartar una cola de recuperación obsoleta.

## Evidencia que cambia la decisión

- Para velocidad, contar consumo de pista en una duración conocida: combinar
  varias frecuencias de entrada/pista y ratios PLL, conservando los bytes de PCM
  originales como control cuando el cambio sólo corrige el mezclador opcional.
- Para continuidad, exigir señal de productor no nula y confirmar que la pista
  prevista se reproduce antes de inyectar la pausa del productor. Llegar a una
  pantalla durante un tramo silencioso no ejercita ese fallo audible.
- Probar starvation parcial/completa, reentrada de PCM, reset, cambio de
  frecuencia durante la rampa, rechazo de salida y petición extrema. Registrar
  recuperaciones aparte del tamaño de cola: PCM sintetizado puede ocultar una
  falta de datos si sólo se observa que la cola sigue llena.
- Separar los resultados de transporte/PCM de los de escucha o loopback del
  dispositivo. Un backend dummy o de archivo comprueba contratos; no acredita
  ausencia audible de tirones en hardware. Registrar ese límite sin bloquear
  las correcciones verificadas de código.
