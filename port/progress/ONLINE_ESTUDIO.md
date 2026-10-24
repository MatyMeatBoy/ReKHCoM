# Online: estudio preliminar (23/10/2026)

Pedido del usuario (23/10): llevar el modo cable link (batallas VS de 2 jugadores) a online moderno manteniendo la
lógica del juego; ver si aplica rollback; estudiar cómo lo hicieron otros (decomps con online, Steam / AppID 480
"Spacewar"); reconstruir en alpha la lógica del multijugador.

## Cómo es el cable en el juego (src/sio)

- `sio.c` es el driver de modo multijugador de GBA (SIOCNT en modo multi, 115200 bps). El padre dispara las
  transferencias con el Timer 3 después de cada VBlank; cada transferencia intercambia una palabra de 16 bits de cada
  consola (SIOMULTI0..3).
- Encima del registro hay una **cola de paquetes**: un paquete son 4 palabras + 1 palabra de checksum (5
  transferencias). `SioQueueSendFrame` encola (32 lugares), `SioSendWord` / `SioRecvWord` los mueven por el cable,
  `SioReadRecvFrame` entrega al juego un paquete con las palabras **de los dos jugadores** de la misma ranura (la
  propia incluida, como en el hardware). Paquetes todo cero no cuentan; sin paquete el juego ve `recvEmpty`.
- Control de flujo propio: si el que recibe acumula más de 3 paquetes pide pausa (bit `SIO_PAUSE_REQUEST`) y el otro
  deja de encolar 5 VBlanks. Errores: checksum, cola llena, timeout (hijo sin transferencias 6 VBlanks).
- Quien lo usa: `chara.c` (`SioRunStateMachine` / `SioTransferFrames` una vez por cuadro, con `gSioSendFrame` y
  `gSioRecvFrame`), y encima `mode_sio.c`, `btl_vs.c`, `mode_vsbattle.c` (la batalla VS, el intercambio de naipes).

Conclusión: el juego ya tolera que un paquete tarde (cola, estado vacío, pausa). La red puede entrar **a nivel de
paquete**, reemplazando solo el transporte (registros, timer, interrupción) y dejando intacta toda la lógica de
`chara.c` / `mode_sio.c` / `btl_vs.c`. Es lo que el usuario pidió ("manteniendo su lógica").

## Opciones

1. **Cable virtual a nivel de registro** (como VBA-M / mGBA con link por red): cada transferencia de 16 bits viaja por
   la red. Fiel pero inviable: ~5 transferencias por cuadro, cada una espera la ida y vuelta (decenas de ms).
2. **Cable virtual a nivel de paquete** (elegida para la alpha): una ranura por cuadro de juego. Cada lado manda su
   paquete de la ranura n (aunque sea vacío) por UDP; cuando tiene el del otro arma el par (jugador 0, jugador 1) de
   la ranura n y lo deja en la cola de recepción del juego, igual para los dos. Con latencia el juego ve más
   cuadros "vacíos" y su propio control de flujo hace el resto. Sin predicción ni resimulación.
3. **Rollback (GGPO / GGRS)**: necesita que lo que se intercambia sean **entradas** y que la simulación sea
   determinista y se pueda guardar/restaurar cada cuadro. Los menús del enlace intercambian comandos (conectar,
   naipes, opciones): ahí no aplica. Pero **la batalla VS sí va por entradas** (`chara.c` `SioKeySyncSend/Recv`,
   comando 0x0ACD): cada consola manda sus teclas, el padre reenvía las de los dos y ambas simulan con las mismas, y
   `AgbMain` no avanza un cuadro sin el paquete (lockstep con demora). Rollback en la batalla es viable: predecir las
   teclas del otro (repetir las últimas), simular, y al llegar las reales restaurar con savestate (`port_state.c`,
   ~400 KB por estado) y resimular. Proyecto aparte, para después de la alpha del cable.

## Transporte y emparejamiento

- **UDP directo / LAN** (alpha): IP:puerto, sin servidor. Winsock en Windows, sockets BSD en Linux/Android/Switch.
  Paquete de red: número de ranura + 5 palabras + reenvío de las últimas ranuras no confirmadas (UDP pierde).
- **Steam** (después): Steamworks `ISteamNetworkingMessages` / `ISteamMatchmaking` (lobbies, invitar amigos, relay de
  Valve que atraviesa NAT). El AppID 480 ("Spacewar") es el de pruebas de Valve: sirve para desarrollar, **no para
  publicar** (todos los juegos de prueba comparten ese AppID y Steam no lo aprueba para distribución). Para publicar
  hace falta AppID propio (Steam Direct, 100 USD) o no usar Steamworks.
- **Sin Steam ni servidor propio**: GameNetworkingSockets (el código abierto de Valve, con ICE/WebRTC para NAT) o
  ENet + un servidor STUN público; para el navegador, WebRTC data channels. Un servidor de emparejamiento gratuito
  (Cloudflare Worker / Durable Object) puede repartir códigos de sala.
- Otros juegos/decomps: los ports de decomps con online (p. ej. los de Mario Kart 64 / Super Mario 64 coop, "sm64ex-coop")
  sincronizan **estado** propio por red, no el cable original; los emuladores (mGBA, VBA-M, NO$GBA) hacen link a nivel
  de registro, con los problemas de latencia de la opción 1. Nuestro caso (protocolo con cola) permite algo mejor que
  un emulador sin reescribir la lógica.

## Plan de la alpha (orden)

1. `port/src/port_netlink.c`: transporte UDP y ranuras; `KHCOM_NETLINK=host:<puerto>` o `join:<ip>:<puerto>` (luego
   en Opciones/launcher). Con la red activa, `SioRunStateMachine` / `SioTransferFrames` (bajo `PLATFORM_PC`) toman
   el handshake y la cola desde la red en vez del hardware; sin red todo queda como hoy.
2. Prueba con dos procesos locales ocultos (127.0.0.1) que entran al menú de enlace y a una batalla VS con
   `KHCOM_KEYS`; latencia y pérdida simuladas (`KHCOM_NETLINK_LAG`, `KHCOM_NETLINK_LOSS`).
3. Interfaz: opción "Enlace en red" con alojar/unirse e IP; luego códigos de sala.
4. Steam (lobby + relay) como transporte alternativo, si se consigue AppID.
5. Rollback solo si la batalla VS se reconstruye sobre entradas (opción 3).

## Estado de la alpha (23/10)

Código: `port/src/port_netlink.c` (transportes), `port/src/port_steamlink.c` (Steam), `src/sio/sio.c` (ranuras, solo
PC y solo con red activa), `port/tools/netlink_relay.py` (relay para VPS). Sin red, el cable queda como siempre.

- Transportes (`netlink.cfg` o `KHCOM_NETLINK`): `steam:host|join` (sala pública Spacewar 480 con etiqueta
  rekhcom, primero la de un amigo; P2P por los relays de Valve), `host:<puerto>` / `join:<ip>:<puerto>` (UDP
  directo), `relay:<servidor>:<puerto>:<CODIGO>:host|join` (VPS, crossplay). El launcher lo elige en Mejoras.
- Probado con dos juegos ocultos: conexión, naipes, opciones VS y batalla sincronizada (las teclas del jugador 1
  mueven su personaje en las dos pantallas). Con 6 cuadros de demora y 10% de pérdida; con 4 de demora + 6 de jitter
  + 5% de pérdida esperó 3-6 cuadros de 3600; con un corte total de 10 s en plena batalla los dos esperan ("Esperando
  al otro jugador... N s") y siguen sincronizados al volver. Relay: los dos se encuentran por código y juegan.
- Lo que se cambió de la lógica del cable, solo en red: el límite de reintentos de la aceptación (10 -> 120
  cuadros), la pausa de control de flujo del cable (apagada: peleaba con el búfer y la batalla esperaba el 23% de
  los cuadros) y los errores de timeout / cola llena / checksum (nunca se reportan: se espera).
- Fallo encontrado por el camino, solo del harness: entrar directo a la batalla de enlace saltaba el
  ResetGameState del título (límites de la arena VS en 0). En juego real no pasa.
- Steam probado hasta cargar la DLL y fallar limpio sin Steam abierto; la sala real necesita dos cuentas (prueba
  del usuario con un amigo: carpetas `ReKHCoM-OnlineAlpha-TU` y `-AMIGO` en la raíz de KHCoMPC).

## Lockstep estricto (23/10 tarde) — por qué y cómo

Hallazgo con el primer cruce PC (x86) contra Android 64 (emulador, mucho más lento): DESYNC en el primer chequeo de la
batalla VS. Las posiciones coincidían; difería el generador aleatorio. El generador es el del cartucho (enteros puros,
igual en todas las plataformas); la causa era otra: el hijo nunca recibió la semilla del padre (ni su VT). Los
pares de datos que llegaban a los dos juegos eran idénticos y en el mismo orden, pero **en distinto cuadro del
juego**: fuera de la batalla el lado que esperaba seguía corriendo cuadros sin datos, y la máquina de estados del
intercambio (chara.c, SioExchangeSend/Recv) avanzaba distinto en cada lado. Con dos PC parejos no se notaba.

Arreglo (como hace el cable real, que sincroniza las dos consolas cuadro a cuadro): **cada "cuadro de enlace" del
juego (cada llamada al cable desde la conexión) crea su ranura N y recibe exactamente el par N − D**, con la misma D
en los dos juegos. Si el par no llegó, el juego espera dentro de ese cuadro (imagen quieta, ventana viva, aviso
"Esperando al otro jugador") en vez de correr un cuadro vacío. Así los dos ejecutan la misma secuencia de cuadros
de GBA y todo lo que sale de ella (RNG, intercambio, batalla) es idéntico sin tocar la lógica del juego.

- D la elige el padre: mide el ida y vuelta (RTT) con los HELLO del handshake (cada uno lleva su reloj y el eco del
  otro) y toma D = (RTT/2 + 25 ms) / 16,7 ms, entre 2 y 12 cuadros; viaja en START y en cada paquete de ranuras.
  `KHCOM_NETLINK_DELAY=<n>` la fija (pruebas). Reemplaza al búfer adaptativo anterior, que no podía ser determinista
  (cada lado ajustaba el suyo).
- HELLO lleva la versión del protocolo (NET_VERSION 2): dos builds de reglas distintas no se conectan y avisan.
- Bugs del cartucho que esto destapó en Android (sin recuperación de fallos): `card_battle_riku.c` cardbattle_0
  escribía en `gCardBattleState` antes de crearlo (el hijo crea primero la tarea del rival): `BIOS_PTR`, como su
  gemela de Sora.
- Pruebas: `sh port/tools/netlink_pair.sh` (dos PC, `LINK_ENV` para lag/jitter/pérdida) y
  `sh port/tools/netlink_android.sh` (emulador o teléfono contra PC por relay; `KHCOM_NETLINK_TRACE=1` registra pares,
  semilla y las partes de cada hash).

## Rollback en la batalla (23/10 noche)

Pedido del usuario: que la batalla se sienta local (como Monster Hunter Rise, sin ver "cómo retrocede") con ping
de 20-150 ms y que siga bien hasta 250 ms en picos. Metas fijadas con él: 20-150 ms local, 150-250 ms bien con
correcciones chicas, 300 ms fuera de alcance.

- **Batalla VS: rollback.** El cuadro de enlace n juega el par n − D con D = 1 (RTT ≤ ~70 ms) o 2; si el paquete
  del otro no llegó se adivina (su último paquete otra vez) y el juego sigue. Al llegar el real distinto, el juego
  vuelve a la instantánea de ese cuadro y re-simula hasta el presente en el mismo cuadro real, sin dibujar, sonar ni
  esperar (`port/src/port_rollback.c`: anillo de 32 instantáneas de ~740 KB tomadas en la espera de VBlank del bucle
  principal, como los estados rápidos; las teclas de cada cuadro se guardan y se repiten).
- **Menús del enlace: lockstep** con la misma D (cambian de pantalla y cargan según lo recibido: no se juegan sobre
  adivinanzas). `VsBattleRunning()` (mode_vsbattle.c) decide; `SYSTEM_FLAG_LINK_ACTIVE` NO sirve: está prendida en
  todo el enlace.
- **Revisiones:** los paquetes del cartucho no son solo teclas (el padre retransmite las de los dos; los menús
  responden a lo recibido), así que un cuadro re-simulado puede producir otro paquete propio: reemplaza al enviado,
  se reenvía en un datagrama propio durante 500 ms (por reloj: un juego que espera no hace cuadros) y el otro lado
  re-simula lo que usó el viejo. Converge porque cada paquete depende solo de cuadros anteriores.
- **Sincronía de tiempo:** el que corre adelantado cede un cuadro de su horario (`PortLinkYieldFrame`) cada 8 cuadros
  mientras su ventaja supere la del otro (viaja en cada paquete), como GGPO. Sin esto uno adivinaba casi todo y el
  otro casi nada.
- **Ventana:** solo se espera si el otro queda más de 12 cuadros atrás (corte real).
- **Chequeo de desync** sobre cuadros ya definitivos (48 cuadros atrás, sin rollback pendiente).
- Medido (dos PC, relay local, simulador por tiempo): RTT ~160 ms con jitter y 1 % de pérdida: **0 cuadros
  esperados en 2400 de batalla**, 47-114 rollbacks, 20/20 chequeos sin desync. Extremo (RTT ~230 ms, jitter de
  hasta 8 cuadros, 5 % de pérdida): 10/10 sin desync, esperas solo en los picos.
- Herramientas: `KHCOM_ROLLBACK_TEST=<n>` fuerza un rollback de 6 cuadros cada n y compara cada cuadro re-simulado
  con el original (primera región/byte distinto al registro; el mezclador de audio no cuenta: no suena al
  re-simular). `port/tools/netlink_cleanup.ps1` mata juegos de prueba colgados (ocupan los puertos).
- Lecciones del camino: el simulador de lag contaba cuadros y un juego que espera no hace cuadros (bloqueo solo del
  arnés): ahora va por reloj. Un reenvío de revisiones medido en cuadros tampoco vencía nunca durante una espera.

## Siguiente (para llegar a "no se nota que es online")

1. Hecho (23/10): hash del estado compartido de la batalla (posición y VT de los dos, en orden de jugador, y los
   generadores aleatorios) cada 120 cuadros, comparado entre los dos: 0 desync en 20+ comprobaciones con los dos
   peleando; `KHCOM_NETLINK_FAKEDESYNC` prueba que salta. Reparar mandando el savestate del anfitrión NO sirve en
   KHCoM: cada máquina guarda el mundo desde su lado (gBtlWork es el jugador local, gRikuBtlWork el otro), el
   estado de una no es válido en la otra sin intercambiar los lados. Haría falta esa transformación; mientras
   tanto el lockstep por teclas con la misma semilla hace la desync imposible y la comprobación lo vigila.
2. Reconexión tras reinicio del juego del otro: mismo esquema (savestate del anfitrión + número de ranura).
3. Rollback en la batalla (predecir las teclas del otro, resimular con `port_state.c`): quita la demora del búfer.
4. VPS: `netlink_relay.py` en cualquier VPS de 4-6 USD/mes (1 vCPU basta: ~10 KB/s por partida). Para salas por
   código desde el juego y crossplay (Android, Switch, Linux): el transporte relay ya funciona en cualquier sistema
   con UDP; falta compilarlo en esos ports y una pantalla para escribir el código. Android: la APK hoy no pide
   el permiso INTERNET (es offline a propósito): agregarlo solo cuando el enlace en red llegue a Android.
