#ifndef PORT_ONLINE_H
#define PORT_ONLINE_H
#include "port_online_protocol.h"

int PortOnlineModern(void);
int PortOnlineVersion(void);
void PortOnlineReset(void);
void PortOnlinePump(void);
/* Heap owned protocol state is deliberately excluded from rollback snapshots. */
ModernPeer* PortOnlinePeer(void);
int PortOnlineError(void);
void PortOnlineFail(int error);

#endif
