#!/usr/bin/env python3
"""Compile the production field-enemy culling functions in a small C fixture.

The macros and function bodies are extracted verbatim from src/map/map_tasks.c,
so this checks their actual comparisons and lifecycle behavior without building
the whole game. Runtime/savestate behavior still needs a separate test.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src/map/map_tasks.c"


def extract_function(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[start:pos + 1]
    raise ValueError(f"Unclosed function: {signature}")


def production_fragment() -> str:
    source = SOURCE.read_text(encoding="utf-8")
    lines = source.splitlines()
    macros = "\n".join(line for line in lines if line.startswith("#define MAP_ENM_OFF_"))
    if macros.count("#define MAP_ENM_OFF_") != 4:
        raise ValueError("Expected all four production MAP_ENM_OFF_* macros")
    funcs = "\n\n".join(extract_function(source, signature) for signature in (
        "s32 MapEnm00CheckOffscreen(",
        "void MapEnm01CheckOffscreen(",
        "void MapEnm04CheckOffscreen(",
    ))
    return macros + "\n\n" + funcs


HARNESS = r"""
#include <assert.h>
#include <stdio.h>
typedef int s32;
typedef struct { s32 x, y, z; } FldPos;
typedef struct { FldPos fieldPosition; } FldObj;
typedef struct { int disabled; } Collider;
typedef struct { FldObj obj; void (*update)(void); Collider collider; } MapEnmWork;
typedef struct { MapEnmWork enm; int wasOnScreen; } MapEnm01Work;
typedef struct { s32 x, y; } FieldState;
static FieldState fieldState;
#define gFieldState (&fieldState)
enum { TRUE = 1, FALSE = 0, DISPLAY_WIDTH = 240, DISPLAY_HEIGHT = 160 };
static int gPortViewLeft, gPortViewTop, gPortViewRight, gPortViewBottom;
#define VIEW_LEFT gPortViewLeft
#define VIEW_TOP gPortViewTop
#define VIEW_RIGHT gPortViewRight
#define VIEW_BOTTOM gPortViewBottom
#define MAP_ENM_VIEW(view, gba) (view)
static void ColliderSetDisabled(Collider* c, int disabled) { c->disabled = disabled; }
static void keep_updating(void) {}

/* Production macros and function definitions are inserted here verbatim. */
__PRODUCTION_FRAGMENT__

static void set_view(int width, int height) {
    int origin_x = (width - DISPLAY_WIDTH) / 2;
    int origin_y = (height - DISPLAY_HEIGHT) / 2;
    gPortViewLeft = -origin_x;
    gPortViewTop = -origin_y;
    gPortViewRight = width - origin_x;
    gPortViewBottom = height - origin_y;
    fieldState.x = fieldState.y = 0;
}

static void test_00(void) {
    MapEnmWork w;
    s32 left = MAP_ENM_OFF_LEFT, right = MAP_ENM_OFF_RIGHT;
    s32 top = MAP_ENM_OFF_TOP, bottom = MAP_ENM_OFF_BOTTOM;
    w.update = keep_updating; w.collider.disabled = FALSE;
    w.obj.fieldPosition.x = left; w.obj.fieldPosition.y = top; w.obj.fieldPosition.z = 0;
    assert(!MapEnm00CheckOffscreen(&w));
    assert(w.update == keep_updating && !w.collider.disabled);
    w.obj.fieldPosition.x = right; w.obj.fieldPosition.y = bottom; w.obj.fieldPosition.z = 0;
    assert(!MapEnm00CheckOffscreen(&w));
    w.obj.fieldPosition.x = left - 1;
    assert(MapEnm00CheckOffscreen(&w));
    assert(w.update == NULL && w.collider.disabled);
    w.update = keep_updating; w.collider.disabled = FALSE;
    w.obj.fieldPosition.x = right + 1; w.obj.fieldPosition.y = 0;
    assert(MapEnm00CheckOffscreen(&w));
    assert(w.update == NULL && w.collider.disabled);
    w.update = keep_updating; w.collider.disabled = FALSE;
    w.obj.fieldPosition.x = 0; w.obj.fieldPosition.y = top - 1; w.obj.fieldPosition.z = 0;
    assert(MapEnm00CheckOffscreen(&w));
    w.update = keep_updating; w.collider.disabled = FALSE;
    w.obj.fieldPosition.y = bottom; w.obj.fieldPosition.z = 1;
    assert(MapEnm00CheckOffscreen(&w));
}

static void test_01_or_04(int family) {
    MapEnm01Work w;
    void (*check)(MapEnm01Work*) = family == 1 ? MapEnm01CheckOffscreen : MapEnm04CheckOffscreen;
    s32 left = MAP_ENM_OFF_LEFT, right = MAP_ENM_OFF_RIGHT;
    w.enm.update = keep_updating; w.wasOnScreen = FALSE;
    w.enm.obj.fieldPosition.x = left - 1;
    check(&w); /* Not yet seen: keep it alive until it enters the viewport. */
    assert(w.enm.update == keep_updating && !w.wasOnScreen);
    w.enm.obj.fieldPosition.x = left;
    check(&w);
    assert(w.enm.update == keep_updating && w.wasOnScreen);
    w.enm.obj.fieldPosition.x = left - 1;
    check(&w);
    assert(w.enm.update == NULL);

    w.enm.update = keep_updating; w.wasOnScreen = FALSE;
    w.enm.obj.fieldPosition.x = right;
    check(&w);
    assert(w.wasOnScreen && w.enm.update == keep_updating);
    w.enm.obj.fieldPosition.x = right + 1;
    check(&w);
    assert(w.enm.update == NULL);
}

int main(void) {
    const int sizes[][2] = {{240,160}, {960,540}, {1920,540}};
    unsigned i;
    for (i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        set_view(sizes[i][0], sizes[i][1]);
        test_00(); test_01_or_04(1); test_01_or_04(4);
    }
    puts("PASS: production culling functions, GBA/16:9/32:9 bounds and lifecycle transitions");
    return 0;
}
"""


def compiler_path(requested: str | None) -> str:
    candidates = [requested, os.environ.get("CC"), shutil.which("clang"), shutil.which("gcc"),
                  r"C:\Program Files\LLVM\bin\clang.exe"]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return candidate
    raise FileNotFoundError("No C compiler found; pass --cc PATH_TO_CLANG_OR_GCC")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", help="C compiler path; defaults to CC, clang, or gcc")
    args = parser.parse_args()
    cc = compiler_path(args.cc)
    fragment = production_fragment()
    with tempfile.TemporaryDirectory(prefix="khcom-culling-") as tmp:
        c_file = Path(tmp) / "culling.c"
        exe = Path(tmp) / "culling.exe"
        c_file.write_text(HARNESS.replace("__PRODUCTION_FRAGMENT__", fragment), encoding="utf-8")
        subprocess.run([cc, "-std=c99", "-Wall", "-Wextra", "-Werror", str(c_file), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
