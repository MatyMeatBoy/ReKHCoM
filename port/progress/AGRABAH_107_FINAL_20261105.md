# Agrabah 107/108: continuidad de fachada, propuesta pendiente

Responde a la revisión 3 de Atlas `34f9930dc77e48dabb91cd71e453e1f2`
(SHA256 d0331525338957ce679c220fa0632a0b0ef2a58a4494f06a2668c777d6550086).
La aprobación histórica del techo no aprueba esta nueva extensión.

El control v2.8 ya conserva el techo amarillo aprobado, distinto de la calle
naranja. Esta propuesta mantiene esa receta, las ventanas y el lado izquierdo.
Corrige los dientes de las fachadas inferiores mediante muestreo por píxel:
`d=2*y-x`, banda 56<=d<536 desde x448..479 y banda 536<=d<576 desde
x256..287. El cambio de donante sigue la esquina real (200,368).

Los cuatro tiles transparentes de BG2 en x496..511/y496..511 dejaban un
escalón de sombra. Sólo en la extensión exterior al viewport original se
completan desde (x-32,y-16), un bloque nativo opaco completo. BG3 y la tinta
ya dibujada se conservan. No cambia ROM, paleta, cámara ni lógica de actores.
El gate por corrida conserva el camino rápido de tiles fuera de estas bandas.

Validación: prueba C del header real (regresiones, 51.456 corridas y fase de
fachada); compilación de preview; EVENT107 y EVENT108, vistas ancha 3440x1440
y Original 1920x1080, fotogramas 100/300/550/800/1000. Los 20 pares conservan
el área nativa incluso con margen de 3 píxeles; los 10 pares Original son
idénticos completos. El modelo de la esquina coincide exactamente con la
captura real. Evidencia privada: `pruebas/atlas-final-20261105/`.

Alcance ejecutado: EU Windows32. No certifica otras plataformas ni todos los
fotogramas. Dos variantes intermedias rechazadas se conservan como evidencia;
no se publicaron en Atlas. Entrega final antes/solución pendiente de valoración.
