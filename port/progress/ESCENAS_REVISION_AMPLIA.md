# Correcciones de escenas fuera del encuadre anterior — 18/10/2026

Estado: implementación para revisión amplia; no cambia el contador de aprobación
visual (004 León aprobada; 057/061/074 mejorables). Publicación autorizada por
el usuario, condicionada al cierre del conjunto de pendientes, no solo estos fondos.

## Cambios

- Nuevo port_scene_wide_tiles.h: coordenadas de mundo, sin depender del zoom.
- 061: introducir la coordenada fuente manteniendo la diagonal (x+P,y+P/2).
  Antes, el borde derecho del suelo cambiaba y en +2, creando escalones y
  adornos sobre la fachada inferior. Separar panel vertical de suelo.
- 057: banda de pared diagonal, ventanas/vegetación como capa y relleno vertical
  distinto. No convertir un bloque completo con ventana en textura vertical.
  El relleno de arena lejano usa filas 28..31, libres de pasto; no recortar manchas.
- 074: pared y máscara por diagonal; completar el primer motivo aceptado y
  repetir piezas enteras con espacio. El donante del motivo azul es x7..14,
  y18..21: la fila16 incluía un fragmento verde de otro motivo. No tomar su
  rectángulo vecino sin inspeccionar qué hay dentro. No añadir fragmentos a la pared.
- Los perfiles existentes llaman al helper para fuera del bloque ocupado.
  No cambia el dispatch del PPU, el evento004 ni el área GBA del renderizador.

## Evidencia privada

En khcom-wide-review/build:

- wide-runtime-review: 8 comparaciones reales, 004/057/061/074 a resolución
  interna 960×540 y 1440×810. No son imágenes pequeñas ampliadas.
- wide-runtime-beach-final: dos comparaciones adicionales de 057 con el último
  ajuste de arena. Sustituyen las primeras imágenes de playa.
- wide-runtime-monstro-final: dos comparaciones de 074 con el donante ya limpio.
  Sustituyen las primeras imágenes de Monstro.
- wide-scene-regional-tests: 23/23 de perfiles, EU/US/JP, Original y León de
  control. Se repite 074 tras limpiar el donante en wide-monstro-final-regions.
- wide-eu, wide-us y wide-jp: binarios aislados; las tres regiones compilan.

Las comparaciones amplias comprueban que el rectángulo GBA permanece idéntico
al ejecutable congelado anterior y que 004 no cambia. No equivalen a revisión
de cada cámara, cada transición/fundido o partida completa. El usuario debe
poder ver los comparativos completos; mantener las versiones anteriores recuperables.

## Integración

Cambios limitados a los cuatro headers de perfil y herramientas/documentación.
Usar el commit de la rama codex/wide-scene-review. No absorber PPU/Darkside
sin commitear del checkout compartido. El PPU integrado desde d606d69ca ya
llama a estos headers; no hace falta añadir otro dispatch.

Después de recoger commits de guardados y savestates del usuario, comprobar
la entrega conjunta y publicar. Checklist único: PENDIENTES_VIGENTES.md.
