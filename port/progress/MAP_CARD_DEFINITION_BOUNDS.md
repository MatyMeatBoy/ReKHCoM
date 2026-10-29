# Native map-card definition bounds

The save-compatible count array contains270 slots, but the current drawable
definition table contains260 entries:26 families of ten values. The original
inventory scan probes a27th family. Source history already labels that read
past the table; its historic “harmless” comment is not a native memory contract.
This is independent of the division compatibility handler and the #480 spring
height omission. No existing reporter crash is attributed to this finding.

The native port now checks the actual definition-table extent when building the
map-card menu, listing/counting owned families and counting zero-value cards.
Adding, querying, removing or summing a family rejects IDs/ranges without valid
definitions. The serialized270-byte count array and its reserved bytes remain
unchanged; the fix neither clears saved data nor substitutes another card.
The regular99-card limit, nine-copy limit and gold-key exception stay intact.
The debug random-card helper retains its original random draw; the common
producer rejects an unsupported result instead of changing the RNG sequence.

All new branches are `PLATFORM_PC` checks shared by native ports. Preprocessing
the ten exercised source functions without that define produces identical
normalized code before/after. This verifies the cartridge branch is unchanged;
it is not a new matching-ROM build or GBA hardware test.

`port/tools/test_mapcard_definition_bounds.py` compiles the actual production
functions using the game's real32-bit types and controlled tables ending at a
no-access page. Before the correction, inventory construction and adding an
unsupported ID hit that page; reserved-slot enumeration/query controls also
fail, while valid-card/capacity controls pass. Afterward all five cases pass,
including optimized compilation and unchanged reserved count bytes. The guard
page is a test precondition, not a claim that the unmodified game crashes on
every machine. No ROM, save or SDL is needed by this probe.

Windows native32 and desktop64 were also rebuilt and exercised with private
copies of the provided USA save: load slot0, open the map-card submenu, capture
it, leave it and exit normally. Both before/after pairs complete, and the clean
save's menu pixels remain identical in each architecture. The original save
stays unchanged. These software-renderer/dummy-audio runs do not certify other
devices or audible performance. The prior scrollbar regression also passes all
812 inventory/scroll combinations.

```powershell
python port/tools/test_mapcard_definition_bounds.py --cc <i686-clang.exe> --out <private-proof>
python port/tools/test_mapcard_definition_bounds.py --cc <i686-clang.exe> --out <optimized-proof> --opt-level 2
```

Use `--baseline-ref b820fcefac95099bc948fe099428e85ec52393bb` to exercise the
previous source without changing a checkout. Its four failing controls are an
expected negative result. This correction is for the next build; immutable
stable2.2.5 and the already delivered tests have not been replaced.
