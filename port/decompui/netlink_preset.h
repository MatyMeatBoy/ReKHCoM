#ifndef REKHCOM_NETLINK_PRESET_H
#define REKHCOM_NETLINK_PRESET_H
/* One explicitly selected beta endpoint, independent of custom relay settings.
   This is a UDP address, not the Easypanel HTTPS control-panel URL. */
static constexpr const char* kNetlinkBetaSpec = "lobby:45.8.207.150:47800";
#endif
