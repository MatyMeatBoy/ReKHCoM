# The network link between an Android emulator/device and a hidden PC game, through a local relay
# (port/progress/ONLINE_ESTUDIO.md). The smoke APK (build_android.py --smoke --smoke-env KHCOM_FAST=1) must be installed.
#   sh port/tools/netlink_android.sh [serial] [pc frames] [android frames]
# PC = player 1 (opens the room), Android = player 2 (joins at 10.0.2.2, the emulator's address for this PC; a real
# device on the same network: RELAY_HOST=<this PC's LAN address>). Logs in build/netlink/{pc,android,relay.log}.
R="$(cd "$(dirname "$0")/../.." && (pwd -W 2>/dev/null || pwd))" # C:/... on Git Bash: the paths stay valid for the exe
export MSYS_NO_PATHCONV=1 # Git Bash: keep the device paths (/data/...) as they are
cd "$R"
SERIAL=${1:-emulator-5554}; PCF=${2:-6000}; ANF=${3:-2500}
ADB="$R/../toolchain/android/sdk/platform-tools/adb.exe"; PKG=com.rekhcom.android.smoke
BUILD=${BUILD:-build/pc-eu}; B=build/netlink; ROOM=EMU$$
rm -rf $B/pc $B/android; mkdir -p $B/pc $B/android
cp $BUILD/khcom.exe $BUILD/SDL3.dll $B/pc/; printf 'config_version=2\ngraphics_mode=1\nview_mode=4\n' > $B/pc/kh_com.ini
KEYS=$(py -3 -c "print(','.join(f'{f}-{f+3}:A' for f in range(200, 3000, 40)))")
a() { "$ADB" -s $SERIAL "$@"; }
a shell run-as $PKG mkdir -p files/roms
a push roms/B8CP.gba /data/local/tmp/B8CP.gba >/dev/null && a shell run-as $PKG cp /data/local/tmp/B8CP.gba files/roms/B8CP.gba
printf 'KHCOM_FAST=1\nKHCOM_TRACE_MODES=1\nKHCOM_NETLINK_TRACE=1\nKHCOM_QUIT=%s\nKHCOM_NETLINK=relay:%s:47801:%s:join\nKHCOM_MODE=60:mode_sio_battle:1\nKHCOM_KEYS=%s\n' \
    $ANF ${RELAY_HOST:-10.0.2.2} $ROOM "$KEYS" > $B/android/env.txt
a push $B/android/env.txt /data/local/tmp/smoke_env.txt >/dev/null && a shell run-as $PKG cp /data/local/tmp/smoke_env.txt files/smoke_env.txt
a shell run-as $PKG rm -f files/kh_com.log files/kh_com_host.log
py -3 port/tools/netlink_relay.py --port 47801 > $B/relay.log 2>&1 & RELAY=$!
(cd $B/pc && SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy KHCOM_NETLINK_TRACE=1 KHCOM_NETLINK=relay:127.0.0.1:47801:$ROOM:host \
    KHCOM_MODE=60:mode_sio_battle:1 KHCOM_KEYS="$KEYS" KHCOM_QUIT=$PCF timeout 900 ./khcom.exe --language es \
    --rom "$R/roms/B8CP.gba" >/dev/null 2>&1; echo "pc exit $?") & PC=$!
a shell am force-stop $PKG
a shell am start --user 0 -n $PKG/com.rekhcom.android.GameActivity --es region eu --es rom /data/user/0/$PKG/files/roms/B8CP.gba >/dev/null
wait $PC
kill $RELAY 2>/dev/null
a shell run-as $PKG cat files/kh_com.log > $B/android/kh_com.log
a shell run-as $PKG cat files/kh_com_host.log > $B/android/kh_com_host.log
a shell am force-stop $PKG
for p in pc android; do echo "== $p"; grep -a "netlink\|mode:" $B/$p/kh_com.log | grep -v "pair\|read" | head -20; done
grep -a "crash" $B/android/kh_com_host.log | head -5
