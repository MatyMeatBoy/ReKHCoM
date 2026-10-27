/*
 * Fault compatibility is host-specific. PS5 installs its bus/division handlers;
 * Windows/Linux x32 hosts cover validated register32 zero divisors when their
 * guest has text metadata. Other accesses remain fatal unless translated at
 * their source (BIOS_PTR). This guest stub does not install a signal handler.
 */
#include "port/port.h"

void PortBiosFaultInstall(void) {
    PortLog("guest: fault compatibility is provided by the selected host; BIOS_PTR covers translated bus reads");
}
