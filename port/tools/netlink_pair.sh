# Two hidden local games linked over UDP (port/progress/ONLINE_ESTUDIO.md): logs and shots in build/netlink/{p1,p2}.
#   sh port/tools/netlink_pair.sh <frames> <keys for both> [direct|relay]
# BUILD=build/pc-eu (the exe), K1=<keys only player 1 gets>, SHOTS=300,900, MODE=60:mode_sio_battle:1,
# OUTSHOTS=<frames>: the window as presented (port layers too), FAKE1=1 (player 1 fakes a desync), LINK_ENV="KHCOM_NETLINK_LAG=6 ..." (both). relay starts netlink_relay.py on 47801.
R="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$R"; B=build/netlink
# games left by an interrupted run hold the ports: gone first
powershell -NoProfile -ExecutionPolicy Bypass -File port/tools/netlink_cleanup.ps1 2>/dev/null
rm -rf $B/p1 $B/p2; mkdir -p $B/p1 $B/p2
BUILD=${BUILD:-build/pc-eu}
for p in p1 p2; do cp $BUILD/khcom.exe $BUILD/SDL3.dll $B/$p/; printf 'config_version=2\ngraphics_mode=1\nview_mode=4\n' > $B/$p/kh_com.ini; done
F=$1; K=$2; HOW=${3:-direct}
run() { (cd $B/$1 && env $LINK_ENV SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy KHCOM_NETLINK=$2 KHCOM_NETLINK_TRACE=1 \
    KHCOM_NETLINK_FAKEDESYNC=$([ $1 = p1 ] && echo "$FAKE1") KHCOM_MODE=${MODE:-60:mode_sio_battle:1} \
    KHCOM_KEYS="$K$([ $1 = p1 ] && echo ${K1:+,$K1})" KHCOM_SHOTS=${SHOTS:-300,900} ${OUTSHOTS:+KHCOM_OUTPUT_SHOT=$OUTSHOTS} KHCOM_WINDOW=1280x720 KHCOM_QUIT=$F \
    timeout 300 ./khcom.exe --language es --rom "$R/roms/B8CP.gba" >/dev/null 2>&1; echo "$1 exit $?"); }
if [ "$HOW" = relay ]; then
    py -3 port/tools/netlink_relay.py --port 47801 > $B/relay.log 2>&1 & RELAY=$!; sleep 1
    run p1 relay:127.0.0.1:47801:PRUEBA1:host & sleep 1; run p2 relay:localhost:47801:PRUEBA1:join & wait %2 %3
    kill $RELAY 2>/dev/null
else
    run p1 host:47777 & sleep 1; run p2 join:127.0.0.1:47777 & wait
fi
for p in p1 p2; do echo "== $p"; grep -a "netlink\|mode:" $B/$p/kh_com.log | grep -v "pair\|read" | head -20; done
