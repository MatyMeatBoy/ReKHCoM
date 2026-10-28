# Basic movie import coverage (#449)

Movie3 resolves to `zz_event_123` at 170 seconds (floor12's charm scene), but the
desktop C++ and Python basic catalogs omitted that movie. Both catalogs now
select the full source movie into `hd/scenes/zz_event_123.mp4`. The game offset
and existing posters at seconds1/171 are unchanged. No codec, compression,
resolution, subtitle selection or source media bytes are changed by this fix.

The legacy C++ full collector also selected base scenes a second time during
directory scanning. A uniqueness guard prevents repeated work, including the
new basic entry. The language-variant coordinator's map and the Python full
producer already de-duplicate their selections.

Both basic buttons now say “movies” instead of “intros”, including Spanish,
French, German, Italian and Japanese plus their two generated catalog rows.
Existing approximate size labels are retained; this change does not introduce
a measured size claim. Regenerating the entire translation catalog exposed
other pre-existing missing rows; those unrelated rows are outside this patch.

## Evidence and limits

- RED: independently specified movie3 coverage and seven-file basic import fail
  against the previous Python/C++ production catalogs.
- Four focused tests pass: game mapping/catalog/poster contract; real Python
  CLI plain/XOR basic/full imports, destination and unchanged source bytes;
  actual legacy C++ collector basic7/scenes5/full8 unique; both UI buttons and
  their translations. Fixtures are generated MP4 boxes, not copyrighted media.
- The historical legacy collector plus the new seven-file catalog fails the
  uniqueness assertion; the guarded collector passes. Its first private test
  attempt could not load the C++ runtime DLL and is retained as a harness error,
  separately from the subsequent statically linked RED control.
- The actual C++ variant coordinator passes: seven files, reuse of complete
  seven-file generations, rejection/rebuild of an older six-file generation
  with the same source fingerprint, full-profile uniqueness, EN/JP, cancellation
  and copy/corrupt-source preservation. Existing native transaction tests (3)
  and poster tests (4) also pass, including private save preservation.

Raw logs are under `pruebas/issue449-base-import-123/`. There is no full GUI
build, real source-library import or actual `_123` playback claim here. No
original ROM/SAV/library, protected installation or published artifact was
modified. Existing users need to rerun the basic import with the corrected
importer and their own source; an already imported folder cannot create the
missing movie without that source. This follows audio PR452 and does not itself
certify a delivered package or reporter confirmation for the whole issue.
