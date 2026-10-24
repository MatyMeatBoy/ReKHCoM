# The online lobby end to end (port/src/port_lobby.c): a local room server (netlink_relay.py on 47801), player 1
# opens a public room from the lobby screen, player 2 finds it in the list and enters, then the link battle.
#   sh port/tools/netlink_lobby.sh <frames> [keys for both]
# BUILD=build/pc-eu; lobby screen shots: build/netlink/p{1,2}/lobby_<tick>.bmp; logs in build/netlink/{p1,p2}.
R="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$R"; B=build/netlink
powershell -NoProfile -ExecutionPolicy Bypass -File port/tools/netlink_cleanup.ps1 2>/dev/null
rm -rf $B/p1 $B/p2; mkdir -p $B/p1 $B/p2
BUILD=${BUILD:-build/pc-eu}
for p in p1 p2; do cp $BUILD/khcom.exe $BUILD/SDL3.dll $B/$p/; printf 'config_version=2\ngraphics_mode=1\nview_mode=4\n' > $B/$p/kh_com.ini; done
printf 'online_name=Sora\n' >> $B/p1/kh_com.ini
printf 'online_name=Riku\n' >> $B/p2/kh_com.ini
F=$1; K=${2:-$(py -3 -c "print(','.join(f'{f}-{f+3}:A' for f in range(200, 1400, 40)))")}
# player 1: confirm on "Open a public room" at tick 40; player 2: down to the first room and confirm at tick 400
run() { (cd $B/$1 && env $LINK_ENV SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy KHCOM_LOBBY=lobby:127.0.0.1:47801 \
    KHCOM_NETLINK_TRACE=1 KHCOM_MODE=60:mode_sio_battle:1 KHCOM_KEYS="$K" KHCOM_LOBBY_KEYS="$2" \
    KHCOM_LOBBY_SHOTS="$3" KHCOM_QUIT=$F timeout 300 ./khcom.exe --language es --rom "$R/roms/B8CP.gba" \
    >/dev/null 2>&1; echo "$1 exit $?"); }
py -3 port/tools/netlink_relay.py --port 47801 > $B/relay.log 2>&1 & RELAY=$!
sleep 1
run p1 "40:3" "20" & sleep 2; run p2 "400:1,410:3" "20,390" & wait %2 %3
kill $RELAY 2>/dev/null
for p in p1 p2; do echo "== $p"; grep -a "lobby\|netlink: \(connected\|the lobby\|vs seed\|battle \|relay room\)\|mode:\|DESYNC\|checks" $B/$p/kh_com.log | head -16; done
