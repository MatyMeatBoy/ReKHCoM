# Toolkit de compilación y espacio (18/10/2026)

## Una sola VM para las consolas

La imagen `D:/01Claude/ReKHCoM-build/ps5/ps5env.qcow2` (Debian 12 x86_64, con red) es la VM de
compilación de consolas. Se arranca con `port/tools/linux_vm_test.py --disk <imagen> --net`
(la carpeta compartida viaja como tar). Debe servir para todas: PS5 (ya instalado),
Vita (VitaSDK + vitaGL), PS4 (OpenOrbis) y Switch (devkitPro). Cada SDK va en su carpeta
propia bajo `/root/consoles/<nombre>` y la caché de descargas bajo `/root/consoles/.deps`,
para no duplicar sistema base. Las imágenes de prueba Linux (debian-12-nocloud-*) siguen
aparte: son de solo lectura (snapshot) y no crecen.

- PS5: `python port/tools/build_ps5_vm.py` (también lo llama `build_all.py --platform all
  --package` en Windows). Compila desde HEAD limpio; tarda más de media hora.
- Vita/PS4/Switch: mismo patrón, un `build_<consola>_vm.py` por consola (por crear).

PS4 (OpenOrbis) y Linux (sysroots i386/armhf/arm64) compilan nativos en Windows con clang
cruzado: no necesitan VM. Android usa su SDK/NDK en `toolchain/android`; web, emsdk en D:.

21/10: la VM usa `discard=unmap` y al apagar borra `/root/share`, `apt-get clean` y `fstrim`
(linux_vm_test.py), así no crece. Se compactó de 15,8 a 10,8 GB con
`qemu-img convert -O qcow2 -F qcow2 -B <debian-12-nocloud-amd64.qcow2> ps5env.qcow2 nueva.qcow2`
(repetirlo si vuelve a pasar de ~14 GB). Dentro sólo quedan: `/opt/devkitpro`,
`/root/consoles/vitasdk`, `/root/ps5-deps`, `/root/gba` + `/root/gba-cache`, `/root/ccache`,
`/root/rk-release` (PS5) y `/root/d` (prueba Vita).

## Reglas de espacio

- Barridos de pruebas (test_battles/events/monkey, debug_kit): una copia del juego por barrido
  en `<work>/_bin` y enlaces duros por caso (`port/tools/case_bin.py`). Antes copiaban 23 MB por
  caso y llenaron C: (17 GB).
- No dejar copias de los videos HD de prueba (`build/hd*`, `D:/Temp/*.mp4`): ~1 GB cada juego.

- `python port/tools/clean_workspace.py` informa (y con `--apply` borra) lo que sobra en
  `build/`: evidencia, pruebas y registros; conserva cachés de compilación, releases,
  `userstates` (savestates del usuario) y `evidence`. Pasarlo a menudo.
- `--installs --keep-installs 3` poda las carpetas ReKHCoM-vX antiguas (respaldar guardados
  antes; solo cuando el usuario lo pida).
- No guardar ROM ni guardados dentro de `build/` salvo `userstates`.
- Worktrees viejos (`khcom`, `khcom-claude`, `khcom-linux`, `khcom-fix`, `khcom-codex-perf`,
  `khcom-wide-review`, `khcom-ps5`): su `build/` se puede vaciar; el código sin commitear
  de `khcom` y `khcom-claude` es WIP viejo: confirmar con el usuario antes de borrar el worktree.
