# Complete scenario inventory for Atlas

The story index contains 197 entries and 62 shared background definitions.
The desk contains delivered/accepted artwork, not a guaranteed complete index
of those events. Belle's library event130 has no current desk entry. Preserve
all accepted artwork, comments and exact build-version lineage.

Use the existing Python tooling and reuse the current isolated integrator
checkout. The release/test517 branch and published artifacts remain frozen.
The user requested autonomous decisions and no workers.

## Design

Build a read-only inventory from `event_ids.h`, `gEventBackgroundDefs` and all
event scripts. Keep each event and its camera/effect rows even when another
event shares its background. Decode camera deadlines with the engine's u16
cast; camera records describe the next target after the previous deadline,
not independent start timestamps. Keep end sentinels separate from useful
capture times. Include the existing authoritative battle scenery cases.

Cross a supplied Atlas state/export by deterministic catalog keys and scene
labels. A background linked to accepted artwork does not prove coverage of
every event, zoom, camera segment or entry/exit. Return ambiguous/manual links
for review; never turn a missing deterministic link into proof of absence.
Output JSON and a compact Markdown board plus suggested logical-frame samples.
Do not write the desk or change approvals from this inventory command.

Use the output to capture current native views and add genuinely missing
references to Atlas as pending. The requested x3/integer2 profile must be
resolved against current engine settings and recorded explicitly; no assumed
legacy setting or single frame may certify that coverage.

## Implementation and verification

- [x] Add failing synthetic tests for shared backgrounds, NULL/absent entries,
  camera deadline decoding/end sentinel, and manual/accepted catalog links.
- [x] Implement the source inventory and bounded capture-plan metadata in
  `port/tools/art_review/scenario_inventory.py`; reuse battle-case discovery.
- [ ] Run the tests and the tool against the real source and a private,
  read-only Atlas snapshot. Verify every indexed event/script is represented,
  including130; retain unknowns instead of skipping them. Review and integrate.
- [ ] Capture representative missing views sequentially using the current
  closed game package. Cross all existing desk records before adding pending
  entries; preserve approvals and record source, profile and screenshot hashes.

An inventory pass is not runtime coverage or completed artwork. Battlefield
and story camera checks remain explicit until exercised. Startup HD credits,
audio continuity and the newly reported door-card one-frame artifact remain
separate work.

Verified source pass: eight synthetic regressions pass after observed failures;
the real source reports zero unresolved items. The union contains197 stable
event names and62 backgrounds: EU has195 runtime entries, US/JP197 because
EU omits two dummy enum/table rows. Library130 therefore runs as event128 in
EU and130 in US/JP. Keep both identities; never index an EU background table
with the name's numerical suffix. Thirteen field-world definitions and209
debug battle entries/thirteen stage selectors are retained per region.

The read-only desk snapshot contains82 families/78 accepted build versions.
The command reuses the desk's approval policy in memory without constructing
its database-owning Store. Family links retain renamed descendants and cycles
are bounded; they do not grant visual approval to another camera path. The
source/export/generator hashes identify this pass. Full camera captures and
pending-reference registration remain unfinished.

Ruling: the historical term "x3/integer2" is represented by current supported
integer presentation profiles, not by restoring the retired integer_zoom key.
Use1920x1080/zoom6 (x3) and1280x720/zoom4 (x2), scale_mode0, record the effective
canvas and add window/ultrawide profiles only where the current engine permits
them. This changes no game defaults. Both main profiles target640x360 logical
coverage; scripted camera zooms and HUD layout remain separate checks.

Library130 is now registered as a pending native full-map reference with the
human screenshot and gallery view. Every previous desk record and all78
accepted selections were compared unchanged. It is explicitly reference-only,
not completed art or runtime coverage. Before more references, also recognize
the existing "Coliseo121/122/124" scene labels to avoid duplicating approved
families during the new source cross-check.
