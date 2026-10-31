# Online moderno beta: reconstrucción del multijugador original

## Mandato y criterio humano

El usuario pide rehacer el online completo para que los dos jugadores mantengan
la velocidad original independientemente del ping dentro de las rutas probadas.
No basta que conecte, que no cierre ni que el contador visual redondee a60.
El referente de experiencia es jugar con personas lejanas sin pausas de la
simulación. Se conserva el VS original; el cooperativo de historia es futuro.

La velocidad objetivo es59.7275Hz, la del juego original. La red puede retrasar
información remota y exigir correcciones, pero no debe bloquear el bucle del
combate ni añadir cuadros para esperar al otro equipo. No se promete conocer
controles antes de recibirlos ni una partida válida sin datos indefinidamente.

## Diagnóstico confirmado sobre main57a9acacf

- src/sio/sio.c conserva ranuras de paquetes del cable, límite predictivo12,
  NetWait y un ajuste que llama PortLinkYieldFrame según ventaja de red.
- PortLinkYieldFrame, en port/src/port_main.c, añade un período completo al
  reloj de presentación. Ese freno es del port online, no del cartucho literal.
- src/sio/chara.c:SioKeySyncSend/Recv hace que el padre reenvíe las teclas de
  ambos; incluso la tecla local atraviesa el esquema de cola y devolución.
- port/src/port_rollback.c ya proporciona snapshots locales y resimulación,
  pero depende del contador de ranuras del cable. También tolera actualmente
  errores de snapshot con avisos y fallback: eso no sirve al nuevo contrato.
- La prueba real por VPS a239.20ms promedio/242.68ms máximo registró8400
  cuadros de batalla y60checks/0desync por jugador, pero ambos promediaron
 59.470Hz: nueve cuadros de~33.485ms en35s con poco trabajo de CPU por cuadro.
  Esa prueba falla el criterio nuevo aunque el juego no esperara paquetes.

## Enfoques

1. **Controles directos + predicción + rollback local (recomendado).** Conserva
   reglas y simulación determinista del VS, admite respuesta local inmediata y
   corrige entradas remotas tardías. Requiere un protocolo propio y garantizar
   snapshots/estado completo. Es el enfoque que encaja con el VS solicitado.
2. **Anfitrión autoritativo + estados e interpolación.** Posible para un futuro
   cooperativo, pero en VS requiere resolver de nuevo predicción, validación de
   impactos y compensación del invitado. No es un atajo para eliminar latencia.
3. Quitar sólo PortLinkYieldFrame es un experimento A/B útil, no la reconstrucción
   solicitada: dejaría teclas reenviadas por el padre y menús/colas del cable.

Se propone1 para la nueva beta. No se atribuye una arquitectura interna concreta
a Monster Hunter Rise sin evidencia pública; su manual sólo confirma la función
cooperativa. GGPO es referencia técnica, no una promesa ni dependencia obligatoria:
su guía también reconoce límites de predicción que pueden detener la simulación.
La implementación debe dimensionar y validar sus límites para240y300ms reales.

## Selección y compatibilidad

El launcher distinguirá **Online clásico** y **Online moderno (beta)**. La beta
mantendrá el VPS ya preparado como conexión predeterminada, sin activar el nuevo
motor para quien no lo elija. Proveedor/transporte y motor online son conceptos
separados: Steam, IP y el relay pueden transportar mensajes, pero no deciden cómo
avanza el combate. El motor clásico queda accesible para control A/B y regresión.

Persistir el motor en configuración propia sin cambiar silenciosamente preferencias
clásicas ni introducir campos que rompan PortConfig/ABI de los huéspedes64bit.
La beta moderna anunciará otra versión de protocolo y no mezclará sus salas con
el motor clásicov3. Regiones y reglas incompatibles se rechazan antes de empezar.

## Arquitectura y límites entre componentes

### Transporte y sesión

Reutilizar los datagramas del transporte existente y el relay opaco: el VPS no
ejecuta el juego ni necesita ROM o partida. Un módulo nuevo de sesión moderna
gestionará identidad de partida, rol, protocolo y reglas, RTT, mensajes de
control fiables, heartbeats y estados de conexión. Su estado queda fuera de los
snapshots locales: un rollback no puede desrecibir paquetes ni reenviar controles
locales diferentes para un cuadro que ya se publicó.

Ninguna llamada de bombeo de red espera un paquete. Se limita el número de
datagramas procesados por cuadro para que una ráfaga no robe el presupuesto de
simulación. Reintentos por reloj monotónico, contadores envolventes validados,
historial redundante y ACK selectivos recuperan pérdida/reordenamiento.

### Salas y preparación asíncronas

Estados: conectando -> sala -> configuración -> listo -> comienzo -> batalla ->
resultado/revancha. El render y la entrada de UI siguen funcionando en todos.
No se lleva la máquina de intercambios palabra-a-palabra de SioExchange a la red.

Mensajes estructurados intercambian datos de personaje, mazo, escenario, handicap,
semilla y aceptación. El anfitrión valida/resuelve la configuración inicial; ambos
confirman el mismo descriptor y hash de reglas antes de fijar el cuadro0. Se
reutilizan las reglas y validadores originales, no sus esperas del cable.

Serializar campos explícitos y enteros de tamaño fijo, sin punteros ni volcar
structs con padding. Los datos de CharaLinkData y mazo son un mapa de campos,
no un ABI de red. No sobrescribir partidas ni importar mazos sin la acción normal
del jugador. El alcance inicial es VS; no añadir ni retirar silenciosamente
funciones regionales del modo clásico o el intercambio de cartas existente.

### Batalla: dos flujos directos de controles

Cada cuadro nuevo publica las teclas de su jugador con su número de cuadro.
Ambas simulaciones consumen la pareja canónica jugador0/jugador1. El jugador local
no espera que el anfitrión le devuelva su propia tecla. Las rutinas originales
SioKeyStateUpdateA/B pueden reutilizarse para transiciones de teclas, pero sin
SioTransferFrames, SIOMULTI, colas de cable ni reenvío de controles por el padre.

Si falta la tecla remota se predice a partir del último control confirmado.
Cuando llega una diferencia, restaurar antes del primer cuadro afectado y
resimular hasta el presente sin volver a presentar ni emitir dos veces sonido,
logros, guardados o resultados. El render ve el resultado del cuadro actual.
El retardo local por ping no se incrementa automáticamente; cualquier margen
opcional debe ser explícito y no servir para ocultar el freno anterior.

No hay PortLinkYieldFrame ni NetWait en la ruta moderna de combate. El reloj de
presentación sigue siendo el local original. La deriva entre relojes se mide
separadamente de la latencia; no se corrige insertando cuadros completos vacíos.

### Historial, snapshots y corrección

Punto de partida técnico propuesto:256cuadros de historial de controles,64
snapshots locales y paquetes<=512bytes con entradas recientes redundantes. Son
decisiones del implementador sujetas a coste real, no garantía de capacidad.
La admisión a la batalla reserva memoria y verifica que rollback esté disponible;
si no lo está, la beta muestra un error antes de jugar y mantiene viva la UI.
Nunca cambia silenciosamente a lockstep.

Auditar que el snapshot incluya toda la simulación, eventos y controles consumidos.
Ampliar el hash canónico de batalla: posiciones/HP/RNG solos no certifican mazos,
acciones, temporizadores y resultados. Comparar por identidad de jugador y cuadro
confirmado, no por puntero, cámara, contador de render ni instante de llegada.

Entradas más antiguas que el historial útil, fallo de snapshot o diferencias
confirmadas de estado no se ignoran. Deben producir una transición explícita de
recuperación/salida de la partida con UI a velocidad completa. La política no
promete simular correctamente una desconexión arbitrariamente larga.

## Experiencia visual y diagnóstico

Predicción y corrección son distintas de una pausa global. Medir la magnitud de
las correcciones remotas además del FPS. Cualquier suavizado visual es sólo de
presentación; no altera colisiones, cartas, HP ni la simulación confirmada para
aparentar fluidez. Mantener visibles ping, estado y profundidad de rollback.

La grabación debe conservar tiempo de pared real. Preferir captura propia del
juego con marcas de tiempo verificables; el grabador actual por cuadros fijos no
puede ocultar esperas. No tomar el ratón/teclado tras el Escape del usuario.

## Pruebas y aceptación

- Unidades del protocolo: inputs inmutables, duplicados, orden inverso, ACK,
  pérdida de ráfagas, paquetes truncados/grandes, wrap, nonce y versión incorrecta.
- Determinismo: reejecución del mismo registro de controles, snapshots completos,
  efectos externos exactamente una vez y hash confirmado idéntico.
- Dos juegos reales por el VPS con retardos añadidos por reloj, no por cuadros.
  RTT objetivo240ms y300ms, después jitter/pérdida y latencias asimétricas.
- Por cada lado:>=99.9% de59.7275Hz en tramos calientes, p99<25ms y ningún hueco
  >=50ms en la prueba de referencia; cero cuadros bloqueados por red, cero
  desync, cero cierre y cero fallback al cable. Medir también lógica/entrada y
  correcciones: no basta el FPS del render.
- Comparar el mismo movimiento/ataques con y sin grabación. Separar conexión,
  carga/resize, combate y desconexión controlada. Conservar datos fallidos.
- Probar200/240/300ms primero enWindowsEU32; ampliar a32/64, regiones y otras
  plataformas sólo con evidencia. No certificar dispositivos o países no usados.

## Integración, permisos y entrega

Sólo animaciones-evi/recomp-games. ERP, otros proyectos, configuración global,
instalación personal, originales y releases existentes quedan protegidos.
El relay actual puede transportar el protocolo nuevo sin reemplazarlo; modificarlo
sólo si una prueba identifica un requisito concreto dentro del mismo servicio.
Sin workers de implementación: el usuario autorizó un worker sólo para limpiar
temporales. Builds locales jobs1/BelowNormal y una carga de prueba a la vez.

Entregas por PR pequeños bajo el claimCAS vps-online-fullspeed-240ms y lock de
integración. No llamar terminado al proyecto por haber quitado sólo el freno.
Nueva beta y vídeo deben tener revisión, hashes y límites de validación explícitos.

## Fuentes primarias y revisión

- GGPO README: https://github.com/pond3r/ggpo/blob/master/doc/README.md
- GGPO guía: https://github.com/pond3r/ggpo/blob/master/doc/DeveloperGuide.md
- Steam transportes: https://partner.steamgames.com/doc/features/multiplayer/networking
- Manual cooperativo MH Rise: https://game.capcom.com/manual/MHRISE/en/switch/page/2/3

Revisión interna de constructor (sin worker por restricción humana): conservar
reglas originalVS y adaptar la frontera de entradas, separar protocolo/memoria
local, no enviar snapshots crudos entre ABI, no fallback silencioso, no garantía
de latencia cero. La integración exacta de UI y el tamaño final de buffers quedan
como decisiones técnicas que deben justificarse con pruebas. Diseño pendiente
de aprobación humana antes de implementar esta reconstrucción completa.
