# Version identity for 2.2.5

The shared release header is now `v2.2.5`. The Windows PE resource matches
`2.2.5.0`, and the Web page's visible/accessibility labels retain Beta while
showing `v2.2.5`. Linux, Android, Switch and game logs consume the header;
PS5 packaging must explicitly receive
`--version 2.2.5`, which produces content version `02.002.005`.

Android's actual official package producer yields version name
`2.2.5-android-beta`, version code `102020599`, application ID
`com.rekhcom.android`. Existing published packages remain immutable.

PS4 and Vita's two-field numeric SFO encoding is a separate packaging change.
Every final package still needs its own source/component identity, hashes and
target checks. Updating this header does not certify a package or reporter fix.
