# Online para juegos de cable link de GBA (guía para otros juegos)

Lo aprendido llevando el cable de KHCoM a internet (`ONLINE_ESTUDIO.md`), escrito para repetirlo en otros juegos
con decomp o recomp (Shining Soul 2, etc.). El problema histórico: el cable espera respuesta en microsegundos
(0 ms) y los emuladores que mandan cada transferencia por red se desincronizan o "desconectan".

## 1. Encontrar la capa correcta

Casi todos los juegos usan la biblioteca MultiSio de Nintendo o una parecida: debajo, registros (SIOCNT,
SIOMULTI0-3) y una interrupción por palabra; encima, una **cola de paquetes** (KHCoM: 4 palabras + checksum, 32 en
cola; SS2: bloques de 24 bytes en paquetes de 32). Lo que el juego realmente consume son paquetes con los datos de
**todos** los jugadores de la misma "ranura" de tiempo (incluido el suyo).

- No emular registros por red (una ida y vuelta por palabra: imposible).
- Reemplazar el transporte debajo de la cola (en KHCoM: `SioRunStateMachine` / `SioTransferFrames`) y dejar
  intacta toda la lógica de encima.

## 2. Ranuras emparejadas (el cable virtual)

- Cada cuadro de juego es una ranura. Cada consola llena la suya con un paquete de su cola (o vacío).
- Se manda por UDP con número de ranura; cuando se tienen las ranuras n de todos, se arma el "paquete del cable"
  de la ranura n y se entrega igual a todos (mismo orden en cada máquina = determinista).
- Reenviar desde la ranura más vieja sin confirmar (cada datagrama lleva hasta 24 y el número de lo que se tiene
  del otro). No producir más allá de lo confirmado (anillo de 64).
- Nada de checksums ni pausas del cable original: la red ya entrega exacto o espera.

## 3. Nunca cortar

- Quitar en modo red todos los errores del cable (timeout, cola llena, checksum, "hardware"). Si falta un paquete,
  el juego simplemente espera (lo hace solo si su bucle es lockstep, como `AgbMain` de KHCoM) y se avisa en
  pantalla "esperando al otro jugador" tras 1 s de silencio real.
- Los límites de reintentos pensados para 0 ms (KHCoM: 10 cuadros para aceptar la conexión) se amplían solo en red.
- El control de flujo del cable (pausa por cola llena) se apaga: choca con cualquier búfer de red.

## 4. Fluidez

- Búfer adaptativo: tras un cuadro sin paquete, retener hasta tener 2-8 pares en mano; sube con llegadas
  irregulares y baja tras un rato estable. Resultado KHCoM: 0,1% de cuadros esperados con jitter y pérdida.
- Si la parte jugable intercambia **entradas** (teclas) y la simulación es determinista (misma semilla), se puede
  agregar rollback (predecir, resimular con savestates) y quitar la demora del búfer. Si intercambia **estado**,
  rollback no aplica; queda la autoridad del anfitrión (ver ShiningSoul2Recomp `docs/MODERN_ONLINE.md`).

## 5. Transportes

Una sola interfaz de datagramas (`PortNetlinkSend/Recv`) con varios caminos: UDP directo, Steam (Spacewar 480 para
desarrollo: sala + P2P con relay de Valve, carga dinámica de `steam_api.dll`, sin SDK en la compilación) y un relay
propio en VPS por código de sala (crossplay entre plataformas). El juego no sabe cuál se usa.

## 6. Cómo probar sin dos personas

Dos procesos ocultos en la misma PC (`KHCOM_NETLINK=host:N` y `join:127.0.0.1:N`), teclas por harness, y
simuladores de red: demora (`_LAG`), jitter (`_JITTER`), pérdida (`_LOSS`) y corte (`_DROP=desde-hasta`). Medir
cuadros esperados en la parte jugable y comparar capturas de los dos lados.
