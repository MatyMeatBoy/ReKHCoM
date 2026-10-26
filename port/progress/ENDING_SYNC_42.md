# Issue #42: sincronización al iniciar una película HD a mitad del archivo

Reporte: duartepedro03, comentario del 26/10/2026 en
https://github.com/MatyMeatBoy/ReKHCoM/issues/42, video https://www.youtube.com/watch?v=WCxyxOOb2fI.
Windows v2.0, películas HD activadas y escenas HD desactivadas. La promesa GBA
termina y comienza `zz_event_151` a 3:42; la imagen va adelantada respecto del audio.
El usuario comprobó que el MP4 fuera del juego funciona normalmente. La apertura y Riku funcionan.

## Causa y reproducción

`IMFSourceReader_SetCurrentPosition` busca un fotograma clave anterior. El lector puede entregar
PCM anterior al segundo solicitado. El reproductor encolaba todo ese PCM, pero fijaba su reloj
en el segundo pedido, ignorando el timestamp de las muestras. La imagen y los subtítulos seguían
ese reloj adelantado mientras sonaba audio anterior.

Medición con Media Foundation nativo, sin ventana ni dispositivo de sonido, sobre los archivos
importados del usuario (sin modificarlos), unidades en segundos:

| Archivo / seek | Primer paquete video | Primer PCM | Desfase del reloj anterior |
| --- | ---: | ---: | ---: |
| `zz_event_151`, 222 | 221.2333333 | 221.2266666 | 0.7733334 |
| `zz_event_123`, 170 | 169.7666666 | 169.4506666 | 0.5493334 |
| H.264 sintético GOP 10 s, 222 | 220.0666666 | 219.9680000 | 2.0320000 |

La inspección independiente de la grabación del reportero encuentra aproximadamente 807 ms
entre imagen y audio, consistente con el preroll de 773 ms más un fotograma. La correlación
del audio contra el archivo fuente permanece constante entre 225 y 285 s; el video conserva
también su desfase. No es deriva progresiva ni un archivo importado dañado.

## Cambio y verificación

El camino Windows descarta PCM anterior al inicio solicitado, recorta el bloque que cruza
ese instante respetando cuadros PCM completos y obtiene el extremo de la cola del timestamp
real de MF. Video, audio y subtítulos comparten así la línea temporal del archivo. No cambia
los MP4, sus puntos narrativos ni el flujo GBA.

- Regresión `python port/tools/test_hd_movie_seek.py`: RED antes del cambio
  (`seek 222 queued 16384 preroll bytes`), GREEN después. Cubre preroll completo,
  bloque que cruza 170 s, primer PCM posterior al seek y reproducción desde cero.
- `python port/tools/check_sources.py`: OK.
- `python -m unittest discover -s port/tools -p test_hd_scene_gate.py -v`: 6/6.
- `port_hdmovie.c` compila para Windows x86 EU contra Media Foundation y SDL reales.
- Ejecución aislada del juego con el MP4 real 151, ambos drivers SDL `dummy`, película 4:
  antes, primer PCM 221.2266666 y reloj base 222; después, primer PCM aceptado procede
  del paquete 221.9946666 recortado 1024 bytes (256 cuadros PCM a 48 kHz estéreo),
  quedando en 222 s dentro de un cuadro PCM. Primer reloj de imagen después: 222.0.
  Ambos recorridos terminan normalmente en los créditos GBA.

Parientes: el mismo camino Windows corrige también 123/170 s y las reproducciones de galería
que reutilizan esos offsets. Apertura/Riku (offset cero) están cubiertos por la regresión sin
preroll. Linux usa dos procesos ffmpeg con `-ss` preciso y recorte propio; no presenta esta causa.
El reproductor Android no aplica todavía estos offsets: es una carencia distinta, registrada
para seguimiento; esta entrega no afirma corregirla ni haber probado hardware Android.

Los archivos de prueba pesados, capturas y trazas quedan en `build/ending-sync` fuera de git.
No se ejecutaron baterías generales ni se publicó release o comentario externo.
