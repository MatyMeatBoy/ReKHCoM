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
# Same selector INI as Windows. Migrate only architecture keys, once per release.
# ReKHCoM.version is generated from port_version.h when packaging.
RUNTIME_RELEASE=$(sed -n '1p' "$DIR/ReKHCoM.version" 2>/dev/null | tr -d '\r')
case "$RUNTIME_RELEASE" in v[0-9]*.[0-9]*) ;; *) RUNTIME_RELEASE= ;; esac
RUNTIME_MIGRATED=0
runtime_migrate() {
    [ -n "$RUNTIME_RELEASE" ] || return 0
    previous=$(sed -n 's/^runtime_auto_version=//p' "$DIR/decompui.ini" 2>/dev/null | tr -d '\r' | tail -n 1)
    [ "$previous" != "$RUNTIME_RELEASE" ] || return 0
    RUNTIME_MIGRATED=1
    if [ -e "$DIR/decompui.ini" ] && { [ ! -f "$DIR/decompui.ini" ] || [ ! -r "$DIR/decompui.ini" ] || [ ! -w "$DIR/decompui.ini" ]; }; then
        echo 'Runtime migration could not be saved; using Automatic for this launch.' >&2
        return 0
    fi
    runtime_temp=$(mktemp "$DIR/.rekhcom-runtime-XXXXXX") || return 0
    if [ -f "$DIR/decompui.ini" ]; then
        if ! sed '/^runtime_mode=/d; /^force_32=/d; /^runtime_auto_version=/d; /^auto64_failed=/d' "$DIR/decompui.ini" > "$runtime_temp"; then
            rm -f "$runtime_temp"; return 0
        fi
    fi
    if ! printf '\nruntime_mode=0\nruntime_auto_version=%s\n' "$RUNTIME_RELEASE" >> "$runtime_temp" ||
       ! mv -f "$runtime_temp" "$DIR/decompui.ini"; then
        rm -f "$runtime_temp"
        echo 'Runtime migration could not be saved; using Automatic for this launch.' >&2
    fi
}
runtime_migrate
[ "$1" = --migrate-runtime-only ] && exit 0
# Shared preference: 0 automatic, 1 force32, 2 force64. CLI overrides are not saved.
MODE=$(sed -n 's/^runtime_mode=//p' "$DIR/decompui.ini" 2>/dev/null | tr -d '\r' | tail -n 1)
case "$MODE" in 1|2) ;; *) MODE=0 ;; esac
[ "$RUNTIME_MIGRATED" = 1 ] && MODE=0
CLI_MODE=
REGION=
ROM=
while [ "$#" -gt 0 ]; do
    case "$1" in
        --force32) CLI_MODE=1; shift ;;
        --force64) CLI_MODE=2; shift ;;
        --auto) CLI_MODE=0; shift ;;
        --region)
            case "$2" in
                eu|us|jp) REGION=$2; shift 2 ;;
                *) echo 'Usage: ReKHCoM.sh [--region eu|us|jp] [game arguments]' >&2; exit 2 ;;
            esac ;;
        *) break ;;
    esac
done
if [ -n "$CLI_MODE" ]; then MODE=$CLI_MODE
elif [ "${REKHCOM_64:-1}" = 0 ]; then MODE=1
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
rotate_session_log() {
    log=$1
    [ -f "$log" ] || return 0
    rm -f "$log.5" || return 1
    previous=4
    while [ "$previous" -ge 1 ]; do
        if [ -f "$log.$previous" ]; then
            [ ! -d "$log.$((previous + 1))" ] || return 1
            mv "$log.$previous" "$log.$((previous + 1))" || return 1
        fi
        previous=$((previous - 1))
    done
    [ ! -d "$log.1" ] || return 1
    mv "$log" "$log.1"
}
rotate_session_log "$DIR/ReKHCoM.log" || echo "Log rotation failed; retaining previous evidence" >&2
exec >>"$DIR/ReKHCoM.log" 2>&1
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

# The 64-bit build (26/10, port/linux64): on x86-64 the game image runs in a native 64-bit host with the system's own
# GPU driver, no 32-bit libraries needed. If it is missing or does not get going, the 32-bit build below runs.
# REKHCOM_64=0 forces the 32-bit build.
AVAILABLE64=0
MACHINE=$(uname -m)
HOST64=
IMAGE64=
LIB64=
case "$MACHINE" in
    x86_64) HOST64="$DIR/bin64/rekhcom64"; IMAGE64="$DIR/bin64/guest-$REGION.bin"; LIB64="$DIR/lib64" ;;
    aarch64|arm64) HOST64="$DIR/binarm64/rekhcom-arm64"; IMAGE64="$DIR/binarm64/guest-$REGION.macho"; LIB64="$DIR/libarm64" ;;
esac
if [ -n "$HOST64" ] && [ -x "$HOST64" ] && [ -f "$IMAGE64" ]; then AVAILABLE64=1; fi
# This x32 guest is compiled with -msse4.1. Never enter it on an older CPU.
if [ "$MACHINE" = x86_64 ] && [ "$AVAILABLE64" = 1 ]; then
    if ! awk '/^flags[[:space:]]*:/ { seen++; found=0; for(i=1;i<=NF;i++) if($i=="sse4_1") found=1; if(!found) bad=1 }
              END { exit !(seen && !bad) }' /proc/cpuinfo; then
        if [ "$MODE" = 2 ]; then
            echo 'Cannot start in 64-bit mode: this game requires a CPU with SSE4.1.' >&2
            exit 2
        fi
        AVAILABLE64=0
    fi
fi
if [ "$MODE" = 2 ] && [ "$AVAILABLE64" = 0 ]; then
    echo "Cannot start in 64-bit mode: no complete native 64-bit runtime for $MACHINE." >&2
    exit 2
fi
if [ "$MODE" != 1 ] && [ "$AVAILABLE64" = 1 ]; then
    echo "ReKHCoM.sh: 64-bit build"
    BOOT=$(mktemp "$DIR/.rekhcom-boot-XXXXXX") || exit 1
    rm -f "$BOOT"
    LD_LIBRARY_PATH="$LIB64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" KHCOM_ROM="$ROM" \
        "$HOST64" "$IMAGE64" --data "$DIR" --boot-status "$BOOT" -- "$@"
    CODE=$?
    ENTERED=0
    [ -f "$BOOT" ] && ENTERED=1
    rm -f "$BOOT"
    if [ "$MODE" = 2 ] || [ "$ENTERED" = 1 ]; then exit "$CODE"; fi
    case "$CODE" in
        2|3|4|126|127) echo "ReKHCoM.sh: 64-bit bootstrap failed ($CODE): starting 32-bit" ;;
        *) exit "$CODE" ;;
    esac
fi

# The universal bundle keeps ARM hard-float binaries and their loader separate.
# A 64-bit ARM kernel may lack AArch32 support: the loader preflight checks this.
case "$MACHINE" in
    aarch64|arm64|armv[5-8]*|armhf)
        GAME="$DIR/binarmhf/khcom_$REGION"
        ARM_LIB="$DIR/libarmhf"
        if [ ! -x "$GAME" ] || [ ! -x "$ARM_LIB/rekhcom" ]; then
            echo "No bundled ARM 32-bit runtime for $MACHINE; use native ARM64 mode when available." >&2
            exit 2
        fi
        if ! "$ARM_LIB/rekhcom" --verify "$GAME"; then
            echo 'ARM 32-bit runtime unavailable: this kernel may not support AArch32.' >&2
            exit 3
        fi
        exec "$ARM_LIB/rekhcom" --library-path "$ARM_LIB/sys:$ARM_LIB" "$GAME" "$@"
        ;;
    x86_64|i?86) ;;
    *) echo "Unsupported Linux CPU architecture: $MACHINE" >&2; exit 2 ;;
esac

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
        # The loader preflight above is the only safe retry boundary. Once the
        # game starts, a quit/crash must never open a second session.
        exec "$SYS_LD" --library-path "$LIBS" "$GAME" "$@"
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
