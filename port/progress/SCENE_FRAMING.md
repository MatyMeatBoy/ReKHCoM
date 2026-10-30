# Scene framing for 2.3

Ajuste is the default, including configurations without `scene_framing`.
Only Auto-view engine scenes use its whole-pixel camera: 1080p gives x6,
720p x4. The complete native240x160 picture remains visible. Dialogue and
HUD retain the user's independent scale; tall-fit dialogue keeps its floating
scale. Extendido preserves the existing camera, with an experimental notice
because some outer artwork remains unfinished. Both launcher and game expose
this setting in six languages. The config struct/host ABI does not grow.

Original/CRT, field, battle, movies and the saved general zoom are unchanged.
Normal story and debug event entry use the same camera policy. Opening options
or leaving the scene removes its derived camera state.

## Honors admission

SCENE_FRAMING_HONORS.json records ten exact current Atlas build-version approvals,
read without modifying Atlas. Historical honors and shared-map variants do not
qualify. The build generator binds each grant to approval metadata and the actual
shared renderer/art recipes. Changed covered source, revision or approval status
falls back to Ajuste until reviewed again. This is intentionally conservative:
a common renderer edit may retire several exceptions. The generator runs before
cached asset reuse; Windows Ninja also tracks its inputs and generated header.
This source gate does not certify arbitrary external runtime asset replacements.

## Evidence

Compiled actual consumers cover config/default/legacy/save/clamp, presenter
camera/UI separation, narrow fit, resize, menu/battle/Original isolation, message
BG/portrait/glyph/task ownership and launcher selection. Policy and presenter
pass with x86 and x64 compilers. Generator mutation checks fail closed.

Native Windows EU32 captures live outside Git in pruebas/scene-adjust-v23:

- run02: 720/1080 camera geometry, exact Extended baseline and honors parity.
- run04-dialogue-split: frame900 UI4/UI6 independence; unchanged world region
  and exact UI4 text crop against Extended.
- run05-controls: options notice, normal story gallery, dynamic720-to-1080
  resize, Original exact parity and narrow-frame presentation.
- Failed run01/debug-mode and run03/dialogue integration evidence is retained.
- Full Windows64 guest/host build passes ABI, mixed-call, mapping and boot tests.

Captures use native game execution with dummy SDL/software, isolated QA data,
one process and BelowNormal priority. They establish framing/output behavior,
not GPU performance or physical Android/Linux compatibility. No installed game,
save, Atlas record or published release is modified.

## Portrait and Windows64 integration follow-up

The user spotted a missing portrait. Earlier UI text-only comparison was
insufficient: the shared screen compositor clipped the larger dialogue canvas
to the close world canvas. The internal canvas now reserves the full horizontal
UI span, while presentation still crops the centre at the whole scene scale.
Narrow scenes also allow that screen span. run09 confirms the complete Cid
portrait visually, its interior exactly matches the old baseline, and the upper
world region exactly matches the previous close camera. Extended and honors
pixel parity remains exact. Internal BMP width may therefore exceed the visible
camera width; it is not the physical field of view.

Native64 capture uses KHCOM_HOST_SHOT, not guest output readback. Its first
capture exposed a separate SDL software geometry coverage failure at1920x1080
(the same patterned texture at960x540 is exact). A compiled actual host consumer
reproduces it. Exact axis-aligned uniform-color shim quads now use SDL's rectangle
copy path on the Windows64 software backend; other geometry and GPU backends
keep the existing route. Regression covers full-frame patterned pixels, horizontal
flip/filter marker and rejecting non-quad/nonuniform geometry.

Final run10 executes the corrected native64 host and guest: scene007 Ajuste
with complete dialogue, scene007 Extendido and honors121 all exit0 and match
final32 output exactly at1080p. Host8cbbbe9b / guest6db3958a; EU32c5cebf75.
Full guest/host rebuild and ABI/boot tests pass after both integration fixes.

Final run11 passes exact physical world parity at odd1281x721 between UIx1
(padding1282, beyond the512 sprite span) and UIx4. run12 passes narrow-frame
presentation with the complete left Yuffie portrait visually inspected, plus
Original Ajuste/Extendido exact output parity. All QA processes have exited.
