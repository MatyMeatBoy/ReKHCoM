# Architecture preference after an update

Desktop launchers compare `runtime_auto_version` in `decompui.ini` with the
packaged release version. A missing or different version resets only the
architecture preference and its old automatic-failure marker to Automatic.
Manual selection after that first launch survives subsequent launches of the
same version. Explicit `--force32`, `--force64`, and `--auto` override the current
launch without replacing the stored selection.

Windows updates the existing selector file with its atomic-save helper. Linux
uses a same-directory temporary file and rename, including direct/Gaming Mode
entry and migration before displaying the Python GUI. Linux packages, including
the nested Anbernic bundle, carry `ReKHCoM.version` generated from
`port_version.h`. Automatic still checks runtime availability; x86-64 also
requires SSE4.1, matching its guest compiler flags. Existing bootstrap fallback
rules remain in place.

An unwritable selector file remains intact. That launch uses Automatic with
the same compatibility checks; the migration cannot persist until the file is
writable. ROMs, saves, HD resources, and game settings are outside this migration.

Validation uses actual C++ file replacement and read-only Windows files,
shell runtime stubs, the real Python GUI entry flow with a dialog stub, and
Linux/Anbernic package fixtures. It covers old/missing epoch, same-version manual
selection, next version, explicit CLI, missing runtime/SSE4.1, and failed rename.
These fixtures do not claim physical hardware or game execution coverage.
