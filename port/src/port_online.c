#include "port/port.h"
#include "port/port_netlink.h"
#include "port/port_online.h"
#include "port/port_online_config.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

static int sModern = -1, sError;
static ModernPeer* sPeer;

int PortOnlineModern(void) {
    if (sModern < 0) {
        const char* env = getenv("KHCOM_ONLINE_ENGINE");
        char value[32] = {0};
        if (env) sModern = PortOnlineParseEngine(env);
        else {
            FILE* f = fopen(PortConfigPath("online-engine.cfg"), "rb");
            if (f) {
                size_t n = fread(value, 1, sizeof(value)-1, f);
                if (!feof(f) || ferror(f)) value[0] = 0;
                value[n] = 0;
                fclose(f);
            }
            sModern = PortOnlineParseEngine(value);
        }
    }
    return sModern;
}

int PortOnlineVersion(void) { return PortOnlineModern() ? MODERN_PROTOCOL : 3; }

void PortOnlineReset(void) {
    free(sPeer);
    sPeer = NULL;
    sError = 0;
}

ModernPeer* PortOnlinePeer(void) { return sPeer; }
int PortOnlineError(void) { return sError ? sError : sPeer ? sPeer->error : 0; }

void PortOnlineFail(int error) {
    if (!error || PortOnlineError()) return;
    sError = error;
    if (sPeer) sPeer->error = (uint8_t)error;
    PortLog("modern online: session error %d", error);
}

static void Send(void* context, const uint8_t* packet, size_t size) {
    (void)context;
    PortNetlinkSend(packet, (int)size);
}

void PortOnlinePump(void) {
    uint8_t packet[MODERN_PACKET_MAX];
    int role, size;
    uint32_t now;
    if (!PortOnlineModern() || PortOnlineError()) return;
    role = PortNetlinkRole();
    if (!role) return;
    if (!sPeer) {
        uint64_t nonce = SDL_GetPerformanceCounter();
        unsigned region = 0;
#if defined(VERSION_US)
        region = 1;
#elif defined(VERSION_JP)
        region = 2;
#endif
        sPeer = (ModernPeer*)calloc(1, sizeof(*sPeer));
        if (!sPeer) { PortOnlineFail(MODERN_ERR_STATE); return; }
        /* Session nonce distinguishes delayed datagrams; it is not authentication. */
        nonce ^= (uint64_t)(uintptr_t)sPeer;
        ModernInit(sPeer, role == PORT_NETLINK_HOST ? 0 : 1, region, nonce ? nonce : 1);
        PortLog("modern online: starting protocol %d, role %d", MODERN_PROTOCOL, role);
    }
    now = PortNetlinkMs();
    /* A busy socket cannot monopolize a render frame. */
    for (unsigned i = 0; i < 128; i++) {
        size = PortNetlinkRecv(packet, sizeof(packet));
        if (size <= 0) break;
        ModernReceive(sPeer, packet, (size_t)size, now);
        if (sPeer->error) break;
    }
    ModernPump(sPeer, now, Send, NULL);
}
