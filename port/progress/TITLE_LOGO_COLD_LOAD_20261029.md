# Cargas de los logos al saltar la intro

El salto temprano al título introducía juntas las cinco piezas del logo, más
sus rótulos. El calentamiento inicial preparaba la ilustración y la luz del
menú, pero no los logos importados. Una reproducción EU64, D3D11 y velocidad
real registró63.5ms en el cuadro183:49.3ms correspondían a siete cargas HD.
Aquella primera cohorte conservaba la ilustración GBA; sirvió para localizar
el coste, no para certificar el título HD completo del usuario.

`PortHdMenuAssetsWarm` prepara ahora las seis entradas importadas cuyo grupo
es2 y nombre empieza por `gTitleLogo`, incluido el logo de fondo. Usa
`SpriteReady`, su validación, confirmación del bridge y LRU habituales. No
añade archivos, caché paralelo, resolución menor, nuevos hilos ni cambios al
reloj o mezclador. HD apagado o renderer ausente no realizan estas cargas.
Una imagen rechazada conserva el fallback normal y no cancela las restantes.
El presupuesto existente sigue limitando qué puede prepararse en cada equipo.

La prueba C ejecuta el calentador real: RED en la base sin precarga; GREEN
para seis logos, guardas, fallo aislado y exclusión de cartas/otras palabras.
Los contratos de carga y fallback siguen cubriendo cabeceras, dimensiones,
presupuesto32/12MiB, recursos fijados y fallos de decode/upload.

Se compararon después cuatro procesos con ilustración y fuentes HD completas:
dos controles con guest/host congelados deff476 y dos candidatos. Terminaron0
y produjeron señal no nula. En el intervalo178..190, las cargas HD pasan de8
a3 en ambos candidatos. Los PCM del productor y sus tasas conservan exactamente
sus bytes. La imagen final no cambia fuera de los dos indicadores luminosos,
que animan según tiempo real; no se afirma paridad completa de esos indicadores.

La memoria disponible llegó a359MiB de16GiB instalados y hubo gran variación
de intervalos/underruns, incluso entre controles. Los resultados NO acreditan
una mejora general de FPS, una latencia máxima ni la eliminación de todos los
tirones de audio. Uno de los controles coincidió con compilación; no se usa
como medición comparable de latencia. Se elimina esta carga del camino visible,
pero falta revisar Moogle y otros consumidores de gameplay, lectura/decode,
subida, presión de memoria y reutilización. El objetivo de presupuesto adaptativo
de PC usa8GiB RAM/2GiB vídeo como referencia, incluyendo equipos inferiores;
este bloque no implementa todavía esa política.

Evidencia privada: `pruebas/v23-prioridades/audio-current`, hashes por componente,
logs/CSV, PNG y PCM, `FULL-ASSET-COMPARISON.json`. El primer control normal quedó
en el selector porque modo0 significa primer arranque: se preservó como fallo
del arnés y el control con modo elegido y HD apagado terminó0. Dos intentos de
compilación usaron intérpretes sin PyYAML; restauraron el WIP y no son fallos del
juego. Un comparador inicial ignoraba diferencias RGB con alfa igual; se
reemplazó por comparación de bytes y máscara explícita sólo para las dos luces.
