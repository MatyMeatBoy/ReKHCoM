#!/bin/sh
# Manual Ports entry for the GA36 Allwinner handhelds.
ports=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P) || exit 1
game="$ports/rekhcomga36"
cd "$game" || exit 1

case "$(uname -m)" in
    armv7*|armv8*|aarch64|arm64) ;;
    *) echo 'ReKHCoM GA36 requires ARMv7 hard-float support.' >&2; exit 2 ;;
esac

export KHCOM_FBDEV="${KHCOM_FBDEV-/dev/fb0}"
if [ -n "$KHCOM_FBDEV" ] && { [ ! -r "$KHCOM_FBDEV" ] || [ ! -w "$KHCOM_FBDEV" ]; }; then
    printf 'ReKHCoM GA36: framebuffer %s is unavailable.\n' "$KHCOM_FBDEV" >&2
    exit 2
fi
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER:-offscreen}"
export SDL_AUDIO_DRIVER="${SDL_AUDIO_DRIVER:-alsa}"
export KHCOM_FIXED_FULLSCREEN=1 KHCOM_FIXED_480P=1
export KHCOM_GA36_HUD_RAISE=1
export KHCOM_PPU_MENU_HEADER_PARALLEL=1 KHCOM_PPU_NEON_TEXT_COVERAGE=1 KHCOM_PPU_TEXT_BUCKETS=1
export KHCOM_FBDEV_DIRECT_GLYPHS=1 KHCOM_FBDEV_DIRECT_WORLD_GLYPHS=1 KHCOM_FAST_MENU_HEADER_PICK=1
export KHCOM_WORLDSELECT_TUTORIAL_TILE_RESERVE=1
export KHCOM_UI_FAST_ART=1 KHCOM_GA36_OPTIONS_2X=1
export KHCOM_FBDEV_CLIPPED_GLYPHS=1 KHCOM_FBDEV_FRACTIONAL_OVERLAY=1
export KHCOM_TALL_FIT="${KHCOM_TALL_FIT:-0}"
export KHCOM_CONFIG="fullscreen=1;view_mode=4;aspect=5;zoom=4;battle_zoom=4;filter=0${KHCOM_CONFIG:+;$KHCOM_CONFIG}"

exec /bin/sh "$game/ReKHCoM.sh" --force32 "$@"
