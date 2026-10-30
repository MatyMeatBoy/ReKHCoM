# Online beta mediante VPS

## Objetivo y alcance confirmado

El launcher ofrece un servidor ReKHCoM beta preconfigurado para el multijugador
original de dos jugadores. Se conserva Steam, conexión directa y relay manual.
El cooperativo se desarrollará por separado en el futuro. Reforzar el relay para
reutilizarlo con otros recomps no implica implementar sus modos de juego.

Sólo se administra `animaciones-evi/recomp-games` en panel.lyf.cl. ERP y todos
los otros proyectos están fuera de alcance, incluso para reinicios o inspección.
No se modifican ajustes globales del VPS ni se publican releases existentes.

## Alternativas y elección

1. Relay UDP existente: reutiliza el protocolo, salas y simulación del juego;
   requiere publicar un puerto UDP. El usuario eligió aprovecharlo y reforzarlo.
2. Transporte web: facilita atravesar redes que bloquean UDP, pero requiere
   adaptar clientes y servidor. Es una evolución posible, no un fallback actual.
3. Sólo Steam: no aprovecha el VPS y mantiene la dependencia de Steam.

## Arquitectura propuesta

Un contenedor de recomp-games ejecuta el relay ReKHCoM sin privilegios y con
límites de CPU y memoria. No necesita ROM, partidas, base de datos ni credenciales
GitHub. Las salas viven en memoria; reiniciar el relay desconecta las partidas.
El host y puerto públicos se confirman mediante pruebas antes de fijarlos en el
launcher. UDP no utiliza las rutas HTTP/HTTPS del proxy del panel.

Otros recomps usarán procesos y puertos independientes dentro del mismo servicio,
con un manifiesto explícito por juego. No se mezclan salas de juegos distintos ni
se promete compatibilidad con clientes o plataformas no probados. No se configura
un puerto futuro sin su juego y protocolo concretos.

## Refuerzo del relay

Validar longitud y contenido de mensajes, roles y códigos. Usar reloj monotónico
para vencimientos; limitar salas, clientes y tráfico; evitar amplificación de
respuestas; rechazar secuestro de roles ocupados; mantener consistentes los índices
de jugadores al cambiar de sala o vencer una sesión. Tratar cambios de dirección
sin convertirlos en un mecanismo para expulsar jugadores. Cubrir estos contratos
con pruebas del servidor y de datagramas reales. No anunciar autenticación ni
cifrado que el protocolo actual no tiene.

Una sala pública sigue siendo pública: el código no es una contraseña. No se
guardan direcciones de jugadores ni datos del juego en logs persistentes.

## Launcher

Añadir `Servidor ReKHCoM — Beta` como selección explícita, sin cambiar la opción
guardada del jugador. Preconfigurar el endpoint verificado y reutilizar el lobby
de salas del juego. Permitir nombre de jugador y conservar los ajustes de latencia
y estadísticas. Persistencia y reapertura deben conservar la selección beta.
No depender de Steam para esta selección. Comprobar plataformas de launcher
existentes antes de anunciar soporte; documentar las que queden pendientes.

## Validación y entrega

Pruebas de entradas inválidas, límites, vencimientos, limpieza de índices, colisión
de roles y aislamiento de salas. Dos clientes deben crear, listar y usar una sala
a través del puerto público. Una prueba de paquetes no certifica una batalla:
validar también conexión y batalla con dos procesos del juego y evidencias de
sincronización. No afirmar alcance mundial medido con una sola ubicación.

Los cambios van en rama y PR, con claim CAS e integración bajo lock. Conservar
releases publicadas y WIP ajeno. Despliegue reversible con imagen/configuración
versionadas y procedimiento para restaurar la revisión anterior.

## Decisiones técnicas del implementador

Reutilizar Python y su biblioteca estándar para el relay inicial; no introducir
cuentas ni servicios adicionales sin necesidad. Elegir límites iniciales modestos
y configurables para el VPS compartido (4 núcleos, 3.8 GB totales observados).
La exposición pública se restringe al puerto del relay del servicio autorizado.
