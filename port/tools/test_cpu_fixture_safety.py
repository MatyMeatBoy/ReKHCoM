"""Prove both CPU test CLIs reject non-fixture data before any writes/builds."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]


def load(path):
    spec=importlib.util.spec_from_file_location(path.stem,path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module


def snapshot(work):
    return {p.relative_to(work).as_posix():p.read_bytes() if p.is_file() else None for p in work.rglob('*')}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--work',type=Path,required=True)
    a=p.parse_args();a.work.mkdir(parents=True,exist_ok=True)
    cpu=ROOT/'port/windows64/test_cpu_requirement.py';launcher=ROOT/'port/tools/test_launcher_cpu_gate.py'
    with tempfile.TemporaryDirectory(prefix='fixture-guard-',dir=a.work) as tmp:
        root=Path(tmp)
        checks=[(cpu,name) for name in ('kh_com.sav','kh_com.ini','scenes_seen.dat')]
        checks += [(launcher,'launcher-fixtures/bin64/'+name) for name in ('rekhcom64.exe','guest-eu.bin','SDL3.dll')]
        for index,(script,name) in enumerate(checks):
            work=root/str(index);path=work/name;path.parent.mkdir(parents=True);path.write_bytes(b'not synthetic fixture data\x00\xff')
            before=snapshot(work)
            # Nonexistent compiler/SDL prove the CLI aborts before compilation.
            command=[sys.executable,str(script),'--work',str(work),'--toolchain',str(root/'missing-toolchain')]
            if script==cpu:command+=['--sdl',str(root/'missing-sdl')]
            run=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
            assert run.returncode!=0 and b'Refusing to overwrite' in run.stdout,run.stdout
            assert snapshot(work)==before,'preflight modified or added files'
        allowed=root/'known-cpu';allowed.mkdir()
        module=load(cpu)
        for name,expected in module.SENTINELS.items():(allowed/name).write_bytes(expected)
        module.preflight(allowed);module.preflight(allowed)
        allowed=root/'known-launcher'
        for name in ('rekhcom64.exe','guest-eu.bin','SDL3.dll'):
            path=allowed/'launcher-fixtures/bin64'/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'fixture')
        module=load(launcher);module.preflight(allowed);module.preflight(allowed)
    print('PASS 6 negative CLI cases: byte-identical data, no files added, no build; exact synthetic fixtures permit rerun')


if __name__=='__main__':main()
