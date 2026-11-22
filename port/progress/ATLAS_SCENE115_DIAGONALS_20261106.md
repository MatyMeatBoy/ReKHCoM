# Escena115: continuar ambas diagonales del zócalo, sin build

Padre Atlas `bb7866a6068d4e478a3e19fc98bc423b`, revisión3, SHA256
`c10b3e934562fcba7eb49a36a5d6c236e04e541ee5ba6997193a178e8d94b819`.
Nubes ce424d37-4f66-46a6-b446-a20ee9e1aee9 y
e297b909-866b-4370-8534-4534e7656b59 del envío20261105-203509-8870c7.
Piden extender las diagonales derecha e izquierda. El padre fue aceptado e
integrado en611; esta propuesta no hereda su aprobación.

La base de pared BG2 ocupa la diagonal nativa de píxel2y+x hasta287. El
selector exterior entraba en el patrón de escotillas desde la diagonal de
tile32 y cortaba la parte inferior de esa base en ambos extremos del mapa.
Antes de elegir escotillas, continuar sólo la banda exterior32<=2ty+tx<36
con sx=txmod8, sy=ty+(tx-sx)/2. El donante queda en columnas0..7/filas12..17,
manteniendo exactamente la misma diagonal y todos los colores originales.

No cambia BG3 ni ninguna entrada dentro del mapa nativo. Permanecen la punta
en640píxeles BG2 transparentes, la omisión del módulo huérfano y el piso limpio
desde diagonal32 ya aceptados. El caller conserva el cuadro GBA nativo.

Verificación sin compilación: regresiones de ambos extremos RED→GREEN por
interpretación del bloque C real.39.424coordenadas coinciden con el modelo;
262selecciones cambian sólo en la banda exterior BG2, con fase exacta. Las
regresiones previas de punta/huérfano/piso siguen pasando. El panel antes del
modelo coincide exactamente con el panel aceptado del padre, mismo espacio
(-80,-80)..(480,320) y escala2. Mapa nativo y guard GBA iguales píxel a píxel.

La comparativa se rotula **propuesta nueva/simulación sin build**. No es una
captura nueva del juego; no certifica SDL, otras cámaras, Original o plataformas.
Fixture C ampliado pero sin ejecutar. No compilador, DLL, juego, imports ni
paquetes. Evidencia privada: `pruebas/atlas-20261106/scene115-diagonals/`.
