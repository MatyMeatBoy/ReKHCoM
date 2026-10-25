# Atlas: escenas y partes de sprites

Instrucciones directas del usuario, 25/10/2026. Seleccion interna; no agregar selectores al producto.

- Comentarios de arena/escenario: presentar la escena y el contexto relevante.
- Comentarios de sprites: identificar primero que pieza esta cortada; no confundir estados de animacion con dibujos ni con recortes distintos.
- Completar solo lo que falta. Conservar las piezas sanas y su composicion de brazos, antebrazos, patas y capas.
- Agrupar un recorte compartido en una propuesta; registrar las poses que lo reutilizan como cobertura interna. No crear una revision por cada estado repetido.
- Presentar la pieza original y su mejora, ademas del personaje montado para evaluar anatomia y costura. Escena con jefe solo como contexto complementario.
- Una silueta conceptual aceptada sirve de base: acabar la extension con contorno, paleta y sombreado coherentes con KHCoM. Una ilustracion generada no demuestra igualdad de pixeles ni integracion de juego.
- Turbo: una pasada y una correccion dirigida; mantener las capturas habituales, sin workers ni baterias generales. Comunicar cada entrega. Consolidar aprendizajes de aprobados/honores solo en las dos ventanas diarias pedidas.

## Malefica: recuento y anatomia

`python port/tools/art_review/maleficent_animation_review.py` extrae los originales desde la ROM local y deja en build/maleficent-native-audit/ cuerpo, montaje y manifest. No distribuye la ROM ni arte extraido.

Resultado EU comprobado: 41 estados, 27 cuerpos distintos, 3 firmas de recorte corporal. La agrupacion usa el borde del cuerpo, no los brazos: cambiar una pieza sana no crea otra propuesta del mismo recorte.

Frame0 carga el banco completo de tiles compartidos. Los siguientes estados actualizan su prefijo y conservan el torso comun; omitirlo produce huecos falsos y un recuento incorrecto.

El croquis humano 98de3d46 corrige la silueta: las DOS alas ya existen; eliminar la punta roja inventada, seguir el lomo naranja bajo y continuar hacia la cola. No dibujar una tercera ala.

La comparativa Atlas 312570a6afda4786b2d7c80ba4c11768 muestra cuerpo original, montaje original y concepto montado del recorte1. Sigue pendiente adaptar ese dibujo a indices/paleta nativos y completar las otras dos variantes. No marcar nubes antiguas resueltas ni heredar aprobacion por este concepto.
