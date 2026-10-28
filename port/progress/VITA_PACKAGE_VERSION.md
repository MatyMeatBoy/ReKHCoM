# Vita release metadata

The Vita builder now passes `APP_VER` and `VERSION` to `vita-mksfoex`, derived
from the game's release header. For v2.2 both SFO strings are `02.02`, matching
the existing PS4 two-component convention. Title ID `RKHC00001`, save paths,
regions, executable code and package artwork stay unchanged.

Previously neither field was supplied. A read-only inspection of the existing
v2.0 VPK found both fields set to `00.00`; its bytes were preserved. An SDK's
default therefore cannot identify the release reliably. The
[VitaSDK implementation](https://github.com/vitasdk/vita-toolchain/blob/master/src/vita-mksfoex/vita-mksfoex.c)
supports explicit string overrides through `-s NAME=STR`.

Validation: the old generated command failed the explicit v2.2 version check;
`python port/tools/test_vita_package_version.py -v` checks the generated SFO
command, release/prerelease normalization, title ID and regional loop without
starting a VM or compiler. This is command-generation evidence, not proof of
a newly built VPK or physical Vita operation. The release audit must read the
SFO back from the final VPK after packaging.

Switch already derives its NACP version from `port_version.h`; no Switch change
is needed for v2.2. PS4 and PS5 also already derive package metadata from the
release version. Console SDK inventory, builds and hardware checks are separate
delivery gates.
