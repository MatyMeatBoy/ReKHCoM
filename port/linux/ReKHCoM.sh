#!/bin/sh
# ReKHCoM for Linux. Runs with its own loader and libraries (lib/), so no
# 32-bit system packages are needed. Put your cartridge dump in roms/ (any
# file name): the region is read from the ROM header.
#   ./ReKHCoM.sh                 first ROM found in roms/
#   ./ReKHCoM.sh --region us     a specific version (eu, us or jp)
DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REGION=
ROM=
if [ "$1" = "--region" ] && [ -n "$2" ]; then
    REGION=$2
    shift 2
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
echo "ReKHCoM.sh: $(date), folder $DIR, region ${REGION:-auto}"

if [ -z "$ROM" ]; then
    # No dump found: the game itself says so in its own window, which Steam's Game Mode shows (zenity is not there).
    echo "Kingdom Hearts: Chain of Memories ROM not found. Copy your cartridge dump (EU, US or JP, 32 MiB) into: $DIR/roms"
    REGION=${REGION:-eu}
    export LD_LIBRARY_PATH="$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    cd "$DIR" || exit 1
    exec "./bin/khcom_$REGION" --data-dir "$DIR" "$@"
fi

# X11/XWayland and PulseAudio/PipeWire are reached through the bundled client
# libraries. The system's 64-bit GPU driver cannot be loaded into this 32-bit
# process, so SDL draws with its software renderer.
# The game's interpreter is lib/rekhcom (the bundled loader), a path relative
# to this folder: run it from here.
# Libraries that SDL opens later (ALSA, PulseAudio, X11) come from lib/ too.
export LD_LIBRARY_PATH="$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$DIR" || exit 1
exec "./bin/khcom_$REGION" --data-dir "$DIR" --rom "$ROM" "$@"
