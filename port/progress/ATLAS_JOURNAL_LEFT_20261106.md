# Diario/Pepito: verde hasta el borde izquierdo, propuesta nueva sin build

Padre Atlas `0f89b2fa9a1443df99343bf8047f9a1a`, revisión2, SHA256
`8a65d8f7191f7e83a1211048d8532b44729a2908c66f3930e674fe6ee7b92d9a`.
Nube `16d72bc8-6fb9-4c9d-9a9f-5dcddd50ce47`: «este creo que debia llenar
has la izquierda si no me equivoco», envío20261105-203916-ac796c.

Interpretación propuesta: llevar el verde exterior de la fila hasta x0 del
viewport, no ampliar el papel blanco. La limpieza aceptada de PR609 sólo
trataba las tres columnas nativas; el muestreador de margen style12 podía
conservar fuera de la imagen una barra antigua. Ahora se rellena el margen
izquierdo hasta el primer borde nativo, con el verde presentado de cada fila,
en las filas16..159. La recuperación tras rechazo limpia también ese margen.

La secuencia de dibujo nativa y derecha permanece igual. Las tres columnas
originales mantienen sus máscaras de propiedad; BG0/objetos/blends/ediciones
conservan sus píxeles. Encabezado y márgenes del rótulo se dibujan después del
relleno. No cambia la fila ABI20, la fuente de colores, el papel, las curvas
aceptadas, la ayuda bajo el libro ni otros menús. Se conserva el gate ancho
previo: no se amplían los casos de margen demasiado estrecho que hacen fallback.

Verificación sin compilador: regresión RED con609 y GREEN del AST actual.
Seis fixtures de escala/origen, incluyendo3.5x fraccional y margen cero;
comandos nativos/derechos exactamente iguales al padre al retirar los nuevos
rellenos de la izquierda.40..75rectángulos en esos controles, menos de100.
Color de fondo por fila comprobado, once rechazos restauran el margen izquierdo
y se verificó el orden posterior de rótulos/encabezado y el header ABI intacto.
El fixture C tiene un control nuevo, pero no se ejecutó.

La imagen es un **estudio nuevo del renderer y propiedad controlada, sin build**,
con antes/solución a la misma escala y colores presentes en el padre. El resto
de barra inicial es un fixture para mostrar el relleno; no es dump real de la
pantalla humana. Faltan máscaras reales, SDL/float32, navegación y plataformas.
No compilador, DLL, juego, imports o paquetes. No hereda aprobación del padre.
Evidencia privada: `pruebas/atlas-20261106/journal-left/`.
