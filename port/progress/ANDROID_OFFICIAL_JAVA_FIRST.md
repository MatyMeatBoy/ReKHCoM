# Official Android Java-first packaging

The Java-first entry was previously selected only by the startup-test producer.
The normal APK builder copied the SDL LauncherActivity MAIN entry unchanged, so
integrated diagnostics source alone did not deliver the promised entry contract.

The official and separate playable builders now reuse the existing
safe_manifest/validate_manifest transform before Gradle. They stage the same
non-test diagnostics Java classes as the verified startup test: Java-only MAIN,
private owned launcher/game processes, the SDL template's complete configChanges
and the protected report provider. The official app ID, signing path, beta label
and versionCode102020599 are unchanged. Separate playable application IDs receive
their own provider authority. Smoke autoplay keeps its existing SDL entry.

The shared helper preserves its historical test label by default; the official
caller explicitly retains its own label. The transformer and validator are
reused, not copied into a second packaging implementation. No native source,
native flags, ROM/save handling or toolchain is changed.

## Validation and provenance

The actual previous manifest fails the shared Java-only-entry requirement. The
new official staging hook passes both normal and python-O controls, including
exact non-test Java staging, required process/config/provider ownership, separate
app identity, patch version, label and unchanged smoke output. Standalone AAPT35
tests supply the package/version/SDK metadata that Gradle normally merges and
confirm StartupDiagnosticActivity is launchable. AAPT is packaging evidence, not
installation or runtime evidence.

Existing startup manifest/launch/owned-return and JVM regressions remain
applicable. The final APK still requires signature, compiled manifest, native
and asset hashes. Physical success of the earlier font test is evidence for that
test, not a runtime pass for newly packaged2.2.5. No new emulator is required or
claimed here.

Native objects already compiled at f28b884a may be reused only after comparing
all native inputs/flags against the final packaging source. Record those two
source identities separately; a Java-only packaging update does not justify
claiming the prior native build came from a later commit.
