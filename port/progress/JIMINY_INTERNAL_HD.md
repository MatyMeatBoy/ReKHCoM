# Diario interno: trabajo2.3 en curso

Primer bloque: registrar los seis dígitos de página con el emisor decimal
compartido. La ruta anterior hacía DMA directo y no proporcionaba significado
al texto HD. Se mantienen las seis copias y el orden; la rama de cartucho
conserva su código anterior. El índice, total, navegación y guardados no cambian.

`test_jiminy_page_digits.py` ejercita el fragmento real del diario y el emisor
real: falla sin los registros HD y pasa999 parejas de índice/total con las
seis copias nativas idénticas. Windows32 y64 recompilados; contratos ABI/boot64
pasan. Capturas reales de raíz, categorías, ataque, ThreeWishes, siguiente
entrada y retorno en32software y64Direct3D11, también en Auto amplio.

Evidencia privada: `pruebas/jiminy23`, `jiminy23-digits`, `jiminy23-wide`.
Los perfiles emplean una copia de partida y desbloqueo del diario sólo en RAM.
`view_mode=0` es Original, no Auto: la prueba amplia debe usar4. Las bandas
negras del primer perfil eran su configuración, no un fallo del diario.

**Pendiente antes de cerrar P2:** cabeceras internas/PAGE, limpieza del libro,
borde derecho redondeado, variantes de páginas y localización. Este bloque
no certifica esos elementos ni otros dispositivos, y no está en los paquetes
2.2.5 existentes. Continuar el diario antes de pasar al audio.
