#!/bin/sh
# Explicit desktop registration only: Steam shortcuts remain owned by Steam.
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P) || exit 1
case "${1:-}" in ''|--no-dialog) ;; *) echo 'Usage: Setup Steam Deck.sh [--no-dialog]' >&2; exit 2 ;; esac
RESULT=$(/bin/sh "$DIR/ReKHCoM.sh" --install-desktop 2>&1)
STATUS=$?
if [ "$STATUS" -eq 0 ]; then
    MESSAGE="ReKHCoM is registered in your application menu.

1. Open Steam in Desktop Mode: Games > Add a Non-Steam Game.
2. Select ReKHCoM from the installed applications and add it.
3. Leave forced compatibility OFF for this native Linux shortcut.
4. Return to Gaming Mode > Library > Non-Steam > ReKHCoM.

Linux starts the game directly. Open Options with Start + Select together
(the bound GBA buttons), or F1 / Escape on a keyboard.
Steam Input: choose a Gamepad layout. Steam's own button opens Steam.

ES: En Steam de escritorio: Juegos > Anadir un juego que no es de Steam >
ReKHCoM. No fuerces Proton. Vuelve al modo Juego > Biblioteca > No Steam.
Opciones: Start + Select juntos (botones GBA asignados), o F1 / Escape.
No hay launcher separado en Linux. Lee LINUX.md para reparar un acceso viejo.

$RESULT"
else
    MESSAGE="ReKHCoM setup failed. No game was started.
Configuracion fallida. No se inicio el juego.

$RESULT"
fi
printf '%s\n' "$MESSAGE"
# KDE is standard on Deck Desktop; other desktops may offer Zenity. No install
# or Steam client operation is performed, and there is a terminal fallback.
if [ "${1:-}" != --no-dialog ]; then
    if command -v kdialog >/dev/null 2>&1; then
        kdialog --title ReKHCoM --msgbox "$MESSAGE" || :
    elif command -v zenity >/dev/null 2>&1; then
        zenity --info --no-markup --title=ReKHCoM --text="$MESSAGE" || :
    fi
fi
exit "$STATUS"
