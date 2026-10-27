"""Emit an opt-in serial PowerShell run plan; never start a game or build here."""
import argparse
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]

def quote(value):
    return "'" + str(value).replace("'", "''") + "'"

def commands(build32, build64, exe64, guest, rom, sprites, assets, evidence, repeats=3):
    result = []
    for repetition in range(repeats):
        order = (32, 64) if repetition % 2 == 0 else (64, 32)
        for runtime in order:
            command = ['python', str(ROOT/'port/tools/profile_hd_resources.py'), '--build', str(build32 if runtime==32 else build64),
                       '--rom', str(rom), '--hd-sprites', str(sprites), '--assets', str(assets), '--frames', '1800',
                       '--warm-start', '300', '--min-warm', '120', '--output-root', str(evidence/f'round-{repetition+1}-runtime-{runtime}')]
            if runtime == 64: command += ['--exe', str(exe64), '--guest', str(guest)]
            result.append(command)
    # Screenshots are separate QA runs, never mixed into the pacing sample.
    for runtime in (32,64):
        base=result[0 if runtime==32 else 1].copy()
        base[base.index('--output-root')+1]=str(evidence/f'quality-runtime-{runtime}')
        result.append(base+['--quality-capture'])
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('build32','build64','exe64','guest','rom','sprites','assets','out'):
        p.add_argument('--'+name, type=Path, required=True)
    p.add_argument('--repeats', type=int, default=3)
    a=p.parse_args()
    if a.repeats<1:p.error('repeats must be positive')
    work=a.out.resolve();work.mkdir(parents=True,exist_ok=True)
    plan=commands(a.build32.resolve(),a.build64.resolve(),a.exe64.resolve(),a.guest.resolve(),a.rom.resolve(),
                  a.sprites.resolve(),a.assets.resolve(),work/'measurements',a.repeats)
    (work/'serial-plan.json').write_text(json.dumps(dict(executed=False, scope='paced outer-VBlank workloads only; UI/media dedicated loops require separate evidence',
        argv=plan, no_concurrent_builds=True, vram_measured=False),indent=2),encoding='utf-8')
    lines=["$ErrorActionPreference = 'Stop'", '# Review inputs and run only after the prerelease lane is idle. No builds.']
    for command in plan:
        lines += ['& '+ ' '.join(quote(arg) for arg in command), "if ($LASTEXITCODE -ne 0) { throw 'Profile failed; preserve artifacts and stop serial plan.' }"]
    (work/'run-serial.ps1').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(work/'run-serial.ps1')
if __name__=='__main__':main()
