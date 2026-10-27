# Vida estable de valores getenv del guest

Claim `guest-getenv-stable-values`, owner `codex-control-prompts`.
Rama `fix/guest-getenv-stable-values`.

`GetenvLocked` tenía 128 entradas y, al llenarlas, reiniciaba `used=0` y
liberaba claves/valores publicados. Un llamador que guardase `KHCOM_QUIT`, un
path o una lista del harness podía conservar un puntero liberado aunque nunca
se hubiese cambiado el entorno. Además publicaba una clave antes de comprobar
sus allocations: strdup fallido dejaba NULL para el próximo strcmp y un fallo
al reservar el valor podía quedar cacheado como ausencia permanente.

La caché nueva conserva sus valores durante toda la imagen: 512 claves, nombres
menores de 256 bytes, buffers de valor de hasta 1 MiB y 4 MiB de heap acumulado.
La tabla tiene 4096 bytes con punteros de 32 bits. Las entradas ausentes solo
conservan su clave; no necesitan sentinel ni buffer persistente. Se publican
clave y valor juntos una vez completos. OOM libera únicamente el intento local
incompleto y permite reintentar, sin mutar entradas previas. Al agotarse el límite
se rechazan claves nuevas con NULL; las conocidas siguen disponibles sin consultar
al host y ningún puntero publicado se libera. No añade sincronización: conserva
el lock existente de getenv. El entorno sigue siendo snapshot por imagen.

Hay 147 claves literales distintas de getenv en port/src, guest/src y src al
hacer el cambio; no es una prueba exhaustiva del conjunto dinámico. Los límites
son guardas de memoria, no una promesa de aceptar un entorno arbitrariamente grande.

## Regresión

`python port/tools/test_guest_getenv_cache.py` compila la función real extraída
de libc.c con CRT/assert nativos y host/allocator de prueba. RED anterior:
retener QUIT=1000 y pedir más de 128 claves invalida su buffer. El allocator de
prueba pone veneno en free pero conserva el bloque hasta terminar el proceso,
para detectar el uso liberado sin que el propio test lea memoria desasignada.

GREEN: 1500 consultas mantienen valor y dirección; tras saturación no hay nuevas
allocations. También fallan selectivamente strdup, primer buffer y crecimiento:
no dejan entradas incompletas ni allocations vivas extra y el reintento funciona.
Un valor de 1000 bytes se conserva completo; valor excesivo se rechaza sin fugas;
valores de 700000 bytes saturan el presupuesto sin superar 4 MiB. El test no usa
ROM ni build del juego. check_sources y diff check pasan.

Este defecto es independiente del cierre temprano del harness Android observado
en el emulador API34. No se ha probado que causara ese cierre. El coordinador
investiga la recreación de Activity tras instalar overlays; no se modifica ni
se relanza la app desde este carril. Sin acceso a la instalación protegida.
