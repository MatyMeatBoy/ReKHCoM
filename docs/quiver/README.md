# ReKHCoM — Quiver submission kit

Status: **submitted by the project owner**, as confirmed in this conversation.
Catalog acceptance and a public listing have not yet been verified.

Submit at <https://quiverlauncher.com/submit>. The former
[tgeorgiadis/quiver-community-app-catalog](https://github.com/tgeorgiadis/quiver-community-app-catalog)
was archived on October 9, 2026 and explicitly no longer reviews issues or pull
requests. Do not open a catalog PR there.

## Catalog copy

**Game:** Kingdom Hearts: Chain of Memories

**Project:** ReKHCoM

**Source:** <https://github.com/MatyMeatBoy/ReKHCoM>

**Type/platform:** PC port; decompilation; GBA; Windows.

Revisit Castle Oblivion in a native PC port of Kingdom Hearts: Chain of Memories,
with widescreen and ultrawide support, optional HD dialogue and menu text, and
support for EU, US and Japanese cartridges.

Launch ReKHCoM.exe and select your own unmodified EU, US or Japanese GBA ROM.
No ROM is included. Native save import is available in the launcher.

The browser version is a separate [Web Alpha](https://matymeatboy.github.io/ReKHCoM/).
Do not advertise Android or native Linux/macOS as released, verified platforms.

## Artwork

| Asset | Dimensions | Use |
|---|---|---|
| [header.png](header.png) | 1920 × 620 | Wide banner |
| [capsule.png](capsule.png) | 600 × 900 | Vertical cover |
| [hero.png](hero.png) | 1920 × 1080 | Library background |
| [logo.png](logo.png) | 1200 × 240 | Transparent wordmark |
| [icon.png](icon.png) | 512 × 512 | App icon |

Editable SVG sources are included. The compositions use the project's existing
public gameplay screenshots, with original vector card/crown/key shapes and text.
Dimensions are supplied as common library-art formats, **not confirmed Quiver
form requirements**; adjust if the authenticated form specifies otherwise.
Public raw URLs and field values are in [submission-draft.json](submission-draft.json).
Screenshots for a gallery: [battle](../screenshots/battle.png),
[field](../screenshots/field.png), [status](../screenshots/status.png),
[ultrawide](../screenshots/ultrawide.png).

These promotional images depict the fan port. Original game characters, art and
marks belong to their respective owners; they are not covered by the source-code
license. The project is not affiliated with Disney or Square Enix. Keep the
existing project and upstream credits.

## Installation metadata

Stable release: **v1.8.3**, Windows x86 ZIP, 21,078,082 bytes.
SHA256: `b3abf8685532393b0f0b4ab090dbc30757cd835b840e5398ecbd7f1dc5f83b51`.

- Repository: `MatyMeatBoy/ReKHCoM`.
- Asset filter for future imports: `*windows-x86*.zip`, matching both the
  historical `windows-x86.zip` and current combined `windows-x86-x64.zip`.
  Quiver supports substring or `*` / `?` wildcard filters, not arbitrary regex.
  See upstream [RepositorySourceHelper.AssetNameMatchesFilter](https://github.com/tgeorgiadis/quiver-launcher/blob/main/lib/QuiverLauncher.Core/Services/RepositorySourceHelper.cs).
  New staging requires only the combined ZIP; do not stage both. This draft
  change does not update the external catalog or alter pinned historical assets.
- Preferred Windows executable: `ReKHCoM.exe`.
- Archive has a single top-level `ReKHCoM-v1.8.3-windows-x86` directory.
- Keep the entire directory structure and DLLs; the regional engines are under
  `bin/`. Do not launch `bin/khcom_eu.exe` directly for initial ROM setup.
- No required cartridge file is bundled. ROM selection happens in our launcher;
  leave Quiver's filesToAdd empty rather than inventing a required file name.
- The current API exposes preferredExecutables. The authenticated form may expose
  this as an advanced option; field availability still needs checking.

[my-app-list.json](my-app-list.json) follows the launcher's documented own-list
format (Settings → Advanced → My app list). It is an optional import, **not a
submission to the public catalog**. Quiver can discover the root executable;
choose ReKHCoM.exe manually if prompted. Installation through Quiver is untested.

## Submission checklist

- [x] Check that ReKHCoM is absent from the public catalog search.
- [x] Verify public release asset, digest, archive structure and launcher name.
- [x] Prepare and visually inspect artwork; validate JSON and image dimensions.
- [ ] Sign in to Quiver and inspect the actual submission form.
- [ ] Apply any field/image requirements from that form.
- [ ] Fill the required AI-use metadata truthfully using the site's definitions.
      Coding assistants contributed substantially; do not select “none”.
- [x] Submitted by the project owner (confirmation in conversation).
- [ ] Record the submission receipt or resulting catalog URL.
- [ ] Test installation and launch from Quiver with a user-supplied ROM.

Research date: 2026-10-16. Public API search for ReKHCoM returned no entries.
The owner submitted through the official website; no Quiver PR was needed.
