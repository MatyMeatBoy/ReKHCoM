# ReKHCoM relay beta

Deploy only to **animaciones-evi/recomp-games**. Never modify ERP, other projects,
global firewall/proxy settings or the VPS host. This service relays the original
two-player link protocol; it does not run game simulation or provide future coop.

Build context contains exactly `Dockerfile` and `netlink_relay.py` (from
`port/tools/netlink_relay.py`). No ROM, game assets, saves, credentials or database.
The Python official base image runs the standard-library relay as UID65532.

Easypanel: upload the ZIP, select Dockerfile build, use one replica, CPU limit0.25,
memory limit128MB. Publish target47800 to published47800, **UDP**. HTTP domains do
not carry this traffic. Don't enable rolling replicas: room state is in memory.
Retain each source ZIP and SHA256; record the pulled base digest and built image.
Rollback by reuploading the previous known-good ZIP and deploying it. Restart or
rollback disconnects all active rooms. This initial service has no prior relay
deployment to restore; stopping only this service disables the beta.

Launcher preset: `lobby:45.8.207.150:47800` (opt-in). Manual room codes use
`relay:45.8.207.150:47800:CODE:host` / `:join`. New room-list clients pad their L
request to512bytes; old two-byte requests receive an empty list without UDP
amplification, but legacy direct room-code connections still work.

Limits:128rooms,2peers/room,16peers/IP,513bytes/datagram,600packets/s/IP,
burst1200,10000packets/s aggregate (burst20000),4096trackedIPs. Individual peer
leases last30seconds, public announcements5seconds. Limits are safety bounds,
not a tested VPS capacity claim. Runtime counters stay in memory; player IPs and
payloads are not logged. No authentication or encryption is implied. Room codes
are public identifiers, not passwords. Occupied roles reject replacement until
expiry, so an address change can require waiting30seconds to reconnect.

Other games need their own process, port and compatible protocol; never point
unrelated recomps at the ReKHCoM namespace. No extra ports are reserved now.

## Research used (30Oct2026)

- [OpenTTD12 multiplayer](https://www.openttd.org/news/2021/09/26/new-multiplayer-experience): coordinator, direct connection/NAT traversal and relay are distinct responsibilities; retain a simple preset that doesn't require router setup.
- [OpenRCT2 FAQ](https://github.com/OpenRCT2/OpenRCT2/wiki/FAQ-%26-Common-Issues): TCP suits its low-update simulation; that is not a reason to replace ReKHCoM's working frame-based UDP/rollback protocol. Avoid privileged runtime users.
- [SM64CoopDX hosting](https://github.com/coop-deluxe/sm64coopdx/wiki/Hosting-and-joining): keep relay-backed lobbies and direct connection separately selectable. No implementation copied.

Web transport and coop remain future changes requiring client/server work; HTTP
hosting alone does not convert UDP games to browser-compatible networking.
