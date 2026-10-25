#!/bin/sh
# ReKHCoM for Linux. Runs with its own loader and libraries (lib/), so no
# 32-bit system packages are needed. Put your cartridge dump in roms/ (any
# file name): the region is read from the ROM header.
#   ./ReKHCoM.sh                 first ROM found in roms/
#   ./ReKHCoM.sh --region us     a specific version (eu, us or jp)
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P) || exit 1

# Desktop Entry Exec has two escaping passes (string, then quoted argument),
# unlike a shell command. In particular, %% is a literal percent sign.
desktop_exec_arg() {
    printf '%s' "$1" | sed -e 's/\\/\\\\\\\\/g' -e 's/["$`]/\\\\&/g' -e 's/%/%%/g'
}
desktop_string() {
    printf '%s' "$1" | sed -e 's/\\/\\\\/g' -e 's/ /\\s/g' -e 's/	/\\t/g'
}
steam_help() {
    printf 'Native Linux shortcut (Compatibility: do not force Proton):\n'
    printf 'Target: /bin/sh\nStart In: "%s"\nLaunch Options: ' "$DIR"
    printf '"%s"\n' "$(printf '%s' "$DIR/ReKHCoM.sh" | sed 's/[\\"$`]/\\&/g')"
    printf 'Keep these quotes. Add game arguments after the quoted script path.\n'
}

case "$1" in
    --install-desktop|--steam-help)
        # Multiline paths cannot be represented safely in Steam's UI fields.
        CR=$(printf '\r')
        case "$DIR${XDG_DATA_HOME:-$HOME/.local/share}" in
            *'
'*|*"$CR"*) echo 'Move the game to a path without newline/carriage-return characters.' >&2; exit 1 ;;
        esac
        if [ "$1" = "--steam-help" ]; then steam_help; exit 0; fi
        APPS=${XDG_DATA_HOME:-$HOME/.local/share}/applications
        case "$APPS" in /*) ;; *) echo 'XDG_DATA_HOME must be an absolute path.' >&2; exit 1 ;; esac
        mkdir -p "$APPS" || exit 1
        ENTRY=$(mktemp "$APPS/.rekhcom-XXXXXX") || exit 1
        trap 'rm -f "$ENTRY"' EXIT HUP INT TERM
        {
            printf '[Desktop Entry]\nType=Application\nName=ReKHCoM\n'
            printf 'Comment=Kingdom Hearts: Chain of Memories (native Linux)\n'
            printf 'Exec=/bin/sh "%s"\n' "$(desktop_exec_arg "$DIR/ReKHCoM.sh")"
            printf 'Path=%s\nIcon=%s\n' "$(desktop_string "$DIR")" "$(desktop_string "$DIR/ReKHCoM.png")"
            printf 'Terminal=false\nCategories=Game;\n'
        } > "$ENTRY" || exit 1
        chmod 644 "$ENTRY" && mv -f "$ENTRY" "$APPS/rekhcom.desktop" || exit 1
        trap - EXIT HUP INT TERM
        printf 'Installed %s\nIn Steam: Games > Add a Non-Steam Game > select ReKHCoM.\n' "$APPS/rekhcom.desktop"
        steam_help
        exit 0 ;;
esac
REGION=
ROM=
if [ "$1" = "--region" ]; then
    case "$2" in
        eu|us|jp) REGION=$2; shift 2 ;;
        *) echo 'Usage: ReKHCoM.sh [--region eu|us|jp] [game arguments]' >&2; exit 2 ;;
    esac
fi

code_of() {
    dd if="$1" bs=1 skip=172 count=4 2>/dev/null
}

for f in "$DIR"/roms/*.gba "$DIR"/*.gba; do
    [ -f "$f" ] || continue
    case "$(code_of "$f")" in
        B8CP) r=eu ;;
        B8CE) r=us ;;
        B8CJ) r=jp ;;
        *) continue ;;
    esac
    if [ -z "$REGION" ] || [ "$REGION" = "$r" ]; then
        REGION=$r
        ROM=$f
        break
    fi
done

# Everything this script and the game print goes to ReKHCoM.log next to it (Steam's Game Mode shows no terminal).
exec >"$DIR/ReKHCoM.log" 2>&1
echo "ReKHCoM.sh: $(date), native Linux, folder $DIR, region ${REGION:-auto}"


if [ -z "$ROM" ]; then
    # No dump found: the game itself says so in its own window, which Steam's Game Mode shows (zenity is not there).
    echo "Kingdom Hearts: Chain of Memories ROM not found. Copy your cartridge dump (EU, US or JP, 32 MiB) into: $DIR/roms"
    REGION=${REGION:-eu}
    set -- --data-dir "$DIR" "$@"
else
    set -- --data-dir "$DIR" --rom "$ROM" "$@"
fi
cd "$DIR" || exit 1
GAME=./bin/khcom_$REGION

# With the GPU (25/10, #140): SDL draws through the system's OpenGL driver, which only loads with the system's own
# C library. SteamOS (and Arch, Debian/Ubuntu with i386 packages) ship the 32-bit loader, C library and Mesa for
# Steam and Proton: when they are there the game starts with them. lib/sys (SDL3) comes first, then the system's
# folders (C library, Mesa, X11); lib/ goes last and only fills what the system lacks (SDL opens X11 and the audio
# libraries itself, so the game's RUNPATH does not reach them; Debian's i386 packages have no libXext, libXcursor
# ... unless asked for, nor the unversioned names SDL opens). Nothing to set: if any piece is missing, or the game
# does not get going, it runs as before with its own loader and libraries (lib/), and SDL draws in software.
# The log says which way it went and which renderer drew ("Renderer:"). REKHCOM_GPU=0 forces the old way.
SYS_LD=
SYS_DIRS=
case "$(uname -m)" in
    x86_64|i?86)
        for d in /usr/lib32 /lib32 /lib/i386-linux-gnu /usr/lib/i386-linux-gnu; do
            [ -d "$d" ] || continue
            SYS_DIRS="$SYS_DIRS:$d"
            [ -z "$SYS_LD" ] && [ -x "$d/ld-linux.so.2" ] && SYS_LD=$d/ld-linux.so.2
        done
        [ -z "$SYS_LD" ] && [ -x /lib/ld-linux.so.2 ] && SYS_LD=/lib/ld-linux.so.2
        ;;
esac
GPU=
if [ "${REKHCOM_GPU:-1}" != 0 ] && [ -n "$SYS_LD" ]; then
    for d in $(echo "$SYS_DIRS" | tr ':' ' '); do
        if [ -e "$d/libGL.so.1" ] || [ -e "$d/libEGL.so.1" ]; then
            [ -e "$d/libX11.so.6" ] || [ -e "$d/libX11.so" ] && GPU=1
        fi
    done
fi
if [ -n "$GPU" ]; then
    LIBS="$DIR/lib/sys$SYS_DIRS:$DIR/lib"
    if "$SYS_LD" --library-path "$LIBS" --list "$GAME" >/dev/null 2>&1; then
        echo "ReKHCoM.sh: GPU, system 32-bit libraries ($SYS_LD$SYS_DIRS)"
        START=$(date +%s)
        "$SYS_LD" --library-path "$LIBS" "$GAME" "$@"
        CODE=$?
        # a normal quit (0) or a run that got going: done. A start that failed at once: the old way below.
        if [ "$CODE" = 0 ] || [ $(($(date +%s) - START)) -ge 8 ]; then
            exit "$CODE"
        fi
        echo "ReKHCoM.sh: the GPU start ended at once (code $CODE): starting with the bundled libraries"
    else
        echo "ReKHCoM.sh: the system's 32-bit libraries do not cover the game: bundled libraries"
    fi
elif [ "${REKHCOM_GPU:-1}" = 0 ]; then
    echo "ReKHCoM.sh: REKHCOM_GPU=0: bundled libraries"
else
    echo "ReKHCoM.sh: no system 32-bit OpenGL (lib32 Mesa): bundled libraries, software drawing"
fi

# The bundled way: the game's interpreter is lib/rekhcom (the bundled loader), a path relative to this folder, and
# the libraries SDL opens later (ALSA, PulseAudio, X11) come from lib/ too.
export LD_LIBRARY_PATH="$DIR/lib/sys:$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$GAME" "$@"
