#!/usr/bin/env python3
"""Exercise Re:CoM save metadata and damage without ROMs or player saves."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include <assert.h>
#include <stdio.h>
#include "port/port.h"
PortConfig gPortConfig;
const char* PortConfigPath(const char* name) { return name; }
void PortVideoApplyConfig(void) {}
void PortLog(const char* fmt, ...) {}
int PortCheatDamage(int damage, int player) { (void)player; return damage; } /* cheats module not linked: neutral */
void PortScenesFromGame(void) {} /* scene gallery not linked */
int PortScaleDamage(int damage, int player);
int PortFastText(void);
void PortRecomSaved(int file);
void PortRecomLoaded(int file);
int main(int argc, char** argv) {
    int mask, fast, bank;
    gPortConfig.saveBank = 1;
    gPortConfig.ps2Mode = 1;
    if (argc > 1) {
        PortRecomLoaded(0);
        assert(PortRecomActive());
        assert(gPortConfig.ps2Difficulty == 2);
        assert(gPortConfig.dodgeButton && !gPortConfig.stockButton && !gPortConfig.unstockButton);
        assert(PortFastText());
        return 0;
    }
    /* Legacy records: bundled controls and invalid difficulty are handled. */
    { FILE* f = fopen("kh_com_recom.ini", "w");
      assert(f); fprintf(f, "slot1=63\nslot2=3\nslot3=579\n"); fclose(f); }
    PortRecomLoaded(0);
    assert(PortRecomActive() && PortQolButtons());
    assert(gPortConfig.ps2Difficulty == 2);
    PortRecomLoaded(1);
    assert(!PortQolButtons());
    gPortConfig.unstockButton = 0;
    PortRecomLoaded(2); /* legacy bit 512 cannot enable the experiment */
    assert(!gPortConfig.unstockButton);
    gPortConfig.unstockButton = 1;PortRecomLoaded(2);
    assert(gPortConfig.unstockButton); /* loading also preserves explicit opt-in */
    for (bank = 1; bank <= PORT_SAVE_BANKS; bank++) {
        gPortConfig.saveBank = bank;
        for (mask = 0; mask < 8; mask++) for (fast = 0; fast < 2; fast++) {
            PortRecomSetActive(1);
            gPortConfig.ps2Difficulty = 2;
            gPortConfig.hdText = 1;
            gPortConfig.dodgeButton = !!(mask & 1);
            gPortConfig.stockButton = !!(mask & 2);
            gPortConfig.unstockButton = !!(mask & 4);
            gPortConfig.ps2FastText = fast;
            PortRecomSaved(-1);
            PortRecomSetActive(0);
            PortSetQolButtons(0);
            gPortConfig.ps2FastText = !fast;
            PortRecomLoaded(-1);
            assert(PortRecomActive());
            assert(gPortConfig.dodgeButton == !!(mask & 1));
            assert(gPortConfig.stockButton == !!(mask & 2));
            assert(gPortConfig.unstockButton == !!(mask & 4));
            assert(PortFastText() == fast);
        }
    }
    assert(PortScaleDamage(0, 1) == 0);
    assert(PortScaleDamage(-1, 1) == -1);
    assert(PortScaleDamage(10, 1) == 15);
    assert(PortScaleDamage(10, 0) == 8);
    gPortConfig.ps2Difficulty = 1;
    assert(PortScaleDamage(1, 1) == 1);
    assert(PortScaleDamage(10, 1) == 5);
    assert(PortScaleDamage(10, 0) == 12); /* Beginner: enemies receive 125%. */
    gPortConfig.ps2Difficulty = 2;
    assert(PortScaleDamage(10, 1) == 15); /* Proud: Sora receives 150%. */
    assert(PortScaleDamage(10, 0) == 8);  /* Proud: enemies receive 80%. */
    /* Version follows the loaded save, not the retired global mode switch. */
    PortRecomSetActive(0);
    assert(PortScaleDamage(10, 1) == 10 && !PortFastText());
    gPortConfig.ps2Mode = 1;
    PortRecomSetActive(0);
    assert(PortScaleDamage(10, 1) == 10 && !PortFastText());
    gPortConfig.saveBank = 1;
    PortRecomSetActive(1);
    gPortConfig.ps2Difficulty = 2;
    gPortConfig.dodgeButton = gPortConfig.unstockButton = 1;
    gPortConfig.stockButton = 0;
    gPortConfig.ps2FastText = 1;
    PortRecomSaved(0);
    return 0;
}
'''


def main():
    tc = next((ROOT.parent / "toolchain").glob("llvm-mingw-*"))
    sdl = next((ROOT.parent / "toolchain").glob("SDL3-*")) / "i686-w64-mingw32"
    with tempfile.TemporaryDirectory(prefix="khcom-recom-") as tmp:
        work = Path(tmp)
        source = work / "test.c"
        source.write_text(HARNESS)
        exe = work / "test.exe"
        subprocess.run([str(tc / "bin/i686-w64-mingw32-clang.exe"),
                        "-I" + str(ROOT / "port/include"), "-I" + str(sdl / "include"),
                        str(source), str(ROOT / "port/src/port_ps2qol.c"),
                        "-L" + str(sdl / "lib"), "-lSDL3", "-o", str(exe)], check=True)
        shutil.copy2(sdl / "bin/SDL3.dll", work / "SDL3.dll")
        env = dict(os.environ)
        env.pop("KHCOM_DAMAGELOG", None)
        subprocess.run([str(exe)], cwd=work, env=env, check=True)
        subprocess.run([str(exe), "reload"], cwd=work, env=env, check=True)
    print("PASS: legacy metadata, 80 control/text combinations, five banks, restart, Beginner/Proud damage, Original isolation")


if __name__ == "__main__":
    main()
