# OptiPort: online beta para 2.3

Pasada del 30 de octubre de 2026. Método verificado contra OptiPort
`42747c240d4e55c142977a40fe4bf51cc25ceac5`; instrucciones instaladas y módulos
general, dominio, profiling y capas de verificación coinciden con upstream.

## Contrato y estado

Conservar controles locales inmediatos e inmutables, orden canónico por jugador,
datos de sesión fuera del snapshot y cadencia original de simulación/presentación.
El selector clásico/moderno es independiente del transporte. Probar ambos peers,
sin certificar gameplay a partir de un test del protocolo.

PR529 sigue en borrador: faltan preparación asíncrona, acuerdo de inicio, routing
de batalla, hash canónico ampliado y prueba ingame por VPS a240/300ms. La prueba
anterior del motor clásico (~239ms) dio59.470Hz frente a59.7275Hz requeridos;
sus0desync/0esperas no acreditan velocidad completa. Sin nuevo vídeo o paquete.

## Cambios verificados en esta pasada

1. No retransmitir controles ya confirmados por el receptor. Se conserva el
   historial completo para replay: recortar el paquete no borra entradas.
   Permanecen la redundancia reciente no confirmada y los rangos de reparación.
   Un heartbeat con todos los controles confirmados pasa de incluir32 entradas
   a una cabecera sin rangos. Prueba de consumidor RED/GREEN.
2. Separar corrección en cola de replay en curso. Retirar una corrección no
   confirma que sus hashes se hayan recalculado. La recepción y envío de hashes
   definitivos quedan suspendidos hasta `ModernFinishRollback`, llamado por el
   puente al alcanzar el presente. Reproducción RED/GREEN del falso desync entre
   dequeue y resimulación; control válido tras recalcular ambos hashes.

## Comparación de tráfico reproducible

Mismo harness, compilador LLVM-MinGW20261006, fuente baseline6be134fda,
1200controles nuevos por peer y bombeo cada17ms, seguido de3s de drenaje cada1ms.
Esto modela el protocolo, no mide el FPS del juego. Contar bytes ofrecidos por
el callback antes de pérdida; excluir handshake, UDP/IP y wrapper del relay.
La carga y el número de datagramas son iguales antes/después.

| RTT modelado | Condición | Bytes antes | Bytes después | Ahorro |
| --- | --- | ---: | ---: | ---: |
| 24ms | Simétrico, sin pérdida | 265612 | 128934 | 51.5% |
| 240ms |45/195ms por dirección, jitter±35ms y ráfagas | 268451 | 205588 | 23.4% |
| 300ms |75/225ms, jitter±35ms, ráfagas y cada13.º datagrama perdido | 271654 | 225508 | 17.0% |

Ráfagas deterministas: descartar40 de cada200datagramas ofrecidos; aplicar por
separado la pérdida cada13 cuando corresponde. Ambos builds reciben finalmente
todos los1200controles por peer y conservan los ACK contiguos. Resultados de
bytes idénticos en ejecutables32/64bits. No se deduce menor RTT, CPU, profundidad
de rollback ni mejora ingame a partir del ahorro de bytes.

Reproducir con `python port/tools/test_online_protocol.py --cc COMPILADOR
--benchmark --report RESULTADO.json`. Para baseline, extraer únicamente
`port/src/port_online_protocol.c` de6be134fda y pasarlo con `--core ARCHIVO` al
mismo harness. JSON registra hash de la fuente y compilador; la comparación usa
las mismas declaraciones de protocolo y exige completar las mismas entradas.

Verificación adicional: suite completa del protocolo (pérdida, reordenamiento,
wrap, historial, nonce/versión/región y hashes), transporte real con seams
deterministas, puente de controles, reserva32/64snapshots y build WindowsEU32.
Estos controles no sustituyen las tareas ingame pendientes.

## Lecciones que se transfieren al método

- Diferenciar transporte, simulación y presentación, y medir cada contrato.
- Separar retención para retransmisión de retención para reconstruir estado.
- Confirmar inputs y terminar replay son condiciones distintas para un hash.
- Validar pertenencia a sesión antes de errores que puedan invalidarla.
- Proteger claves publicadas e historial circular ante replay y frames futuros.
- Probar el bombeo con la cadencia real del consumidor, además de jitter,
  asimetría, pérdidas y wrap; no convertir una prueba simulada en certificado.

Alcance VPS limitado a animaciones-evi/recomp-games; esta pasada no cambia el
servidor. Instalación, ROMs, partidas, releases, ERP y otros proyectos intactos.
