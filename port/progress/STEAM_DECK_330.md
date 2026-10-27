# Steam Deck Gaming Mode y opciones — issue330

Carril `steamdeck-gaming-mode-330`, owner `codex-deck-gaming330`, base e71c9692c.

El reporte describe que ReKHCoM.sh funciona en escritorio, abre directamente
el juego y no encuentra cómo añadirlo a Gaming Mode. No contiene log de un
intento de arranque desde Gaming Mode: no atribuirlo a SDL, Wayland o Gamescope.

El wrapper ya incluye registro XDG con rutas absolutas y campos manuales
--steam-help (issue49). Linux no incluye el launcher C++ de Windows; Opciones
se abre con Start+Select GBA o F1/Escape, comprobado en port_input/port_ui.
La guía LINUX.md ya se empaquetaba; LEEME no explicaba el flujo nativo y
prominenciaba Windows/Proton. Esta entrega mejora descubrimiento, no añade
una segunda GUI ni cambia el renderer.

Se distribuye Setup Steam Deck.sh ejecutable. Registra la app mediante el
wrapper existente y presenta pasos bilingües con kdialog, zenity sin markup,
o salida de terminal. No arranca juego ni modifica Steam, no instala paquetes,
no requiere ROM; conserva el código de fallo del registro. El usuario añade
ReKHCoM desde aplicaciones instaladas en Steam de escritorio y luego abre
Biblioteca/No Steam en Gaming Mode, sin forzar Proton en el acceso Linux.
La guía detalla Opciones, Steam Input Gamepad, ruta manual y reparación después
de mover carpeta. No cambia la ruta PortMaster/Anbernic.

Fuente primaria consultada27/10/2026: [Valve Desktop FAQ](https://help.steampowered.com/en/faqs/view/671A-4453-E8D2-323C/)
confirma añadir aplicaciones instaladas desde Steam Desktop y encontrarlas
en Non-Steam en Gaming Mode. Los campos de acceso derivan del wrapper probado;
no se afirma automatización oficial para editar shortcuts.vdf.

Pruebas: wrapper real con stubs, rutas con espacios/metacaracteres, cwd ajeno,
ROM ausente, EU/US/JP, modos32/64, logs, registro sin iniciar ni tocar Steam;
paquete real tar/zip con binarios mock y permisos0755 del helper. La suite
GLib del wrapper sólo se ejercita automáticamente cuando se ejecuta en Linux
con gio; la corrida Windows/Git sh no prueba el parser de Steam ni Gamescope.
Siguiente prueba mínima: extraer en Deck, Execute helper, añadir app instalada,
Gaming Mode con Gamepad, abrir Opciones y revisar logs/cerrar. Hardware real
pendiente; sin comentario externo ni cierre del issue desde este carril.
