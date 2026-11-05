# Escena115: tres detalles, propuesta sin build

Seguimiento de Atlas `1bf2e47af7fa4b2fa02599248052a27a`, revisión4,
SHA256 `78f75648b1f8c3189084e75bd81f8e703e7feb62ca6514d83346a79e5f425a29`.
Su versión base se integró en PR599. Esta corrección no hereda su aprobación.

- El piso exterior BG3 pasaba a madera limpia en diagonal36; BG2 acababa en32.
  Alinear el comienzo del donante limpio en32 retira los fragmentos rojos.
- La escotilla exterior superior derecha cruza el mapa. Su punta izquierda
  ocupa sólo relleno BG2 transparente en tiles x30..31/y5..9; continuar el
  bloque completo desde(x-24,y+12). Los640píxeles originales son transparentes.
- La escotilla centrada en(tile2,31) no existe dentro del mapa: su extensión
  izquierda quedaba sola. Omitir ese módulo entero, sin borrar otra escotilla.

El sampler se usa fuera del cuadro GBA; la ruta nativa no dibuja esta punta
ni se altera. Ventanas, objetos únicos, paleta y otras escenas se conservan.
Las recetas sólo seleccionan tiles originales, sin bitmaps ROM en Git.

Verificación sin compiladores: interpretación Python del AST del bloque real
de C, tres regresiones RED→GREEN y35.840coordenadas iguales al modelo. Los tres
recortes del antes coinciden exactamente con la captura histórica115/600,
tras la conversión RGB555 del PPU. La simulación conserva el área GBA de esa
cámara. No se ejecutó DLL, juego, compilador ni regeneración de imports.

La nueva imagen Atlas es una **simulación sin build**, con antes/solución en
el mismo encuadre y escala. No demuestra otro fotograma, cámara, modo Original
ejecutado o plataforma. Se actualizó el fixture C, pero queda sin ejecutar
por la restricción humana. Prueba de juego y aprobación de la hija pendientes.
Evidencia privada: `pruebas/atlas-siguiente-20261105/scene115/`.
