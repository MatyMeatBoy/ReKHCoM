#!/usr/bin/env python3
"""Basic native desktop launcher. Own files only; never downloads dependencies."""
import argparse
import importlib.util
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import uuid

if sys.version_info<(3,9):
    print('Native importer needs Python3.9+; use ReKHCoM.sh to play. Nothing was changed.',file=sys.stderr)
    raise SystemExit(2)

KINDS={'music','movies','scenes','sprites','subs','media'}
META={'quality','import.list','.pack'}
class ImportErrorSafe(Exception):pass

def scan(root):
    count=0
    for path in root.rglob('*'):
        if path.is_symlink():raise ImportErrorSafe('Symbolic links are not accepted in HD imports.')
        rel=path.relative_to(root)
        if len(rel.parts)>12:raise ImportErrorSafe('HD directory nesting is too deep.')
        if path.is_file():
            count+=1
            if count>10000:raise ImportErrorSafe('Too many HD files.')
            if rel.parts[0] not in KINDS and str(rel) not in META:raise ImportErrorSafe('Unexpected file in HD folder: '+str(rel))
            if path.suffix.lower() in {'.gba','.sav','.ini','.cfg','.khst','.exe','.so','.dll','.sh','.py'}:raise ImportErrorSafe('Protected/non-HD file rejected: '+str(rel))
            if any(part.startswith('.stage') for part in rel.parts):raise ImportErrorSafe('Unfinished source import; use a completed HD folder.')
            yield path,rel

def _mp4_complete(stream,at,size,seen):
    if not {b'moov',b'mdat'}<=seen:return False
    if at==size:return True
    # Legacy source exports can retain CD fill up to a 16-byte boundary.
    # Accept it only after complete boxes, never inside a truncated box.
    left=size-at
    if size%16 or not 0<left<16:return False
    stream.seek(at)
    return stream.read(left)==b'\xcd'*left

def mp4(path):
    size=path.stat().st_size;at=0;seen=set()
    with path.open('rb') as stream:
        for _ in range(4096):
            if _mp4_complete(stream,at,size,seen):return True
            stream.seek(at);header=stream.read(8)
            if len(header)!=8:return False
            length=int.from_bytes(header[:4],'big');kind=header[4:]
            if length==1:
                extra=stream.read(8)
                if len(extra)!=8:return False
                length=int.from_bytes(extra,'big');minimum=16
            else:minimum=8
            if length==0:length=size-at
            if length<minimum or length>size-at:return False
            seen.add(kind);at+=length
        # The final allowed box may end at EOF or before legacy alignment fill.
        # Keep the same 4096-box limit while accepting that complete boundary.
        return _mp4_complete(stream,at,size,seen)

def validate(root):
    files=list(scan(root))
    if not files:raise ImportErrorSafe('No HD files were produced/found.')
    for path,rel in files:
        if path.suffix.lower()=='.mp4' and not mp4(path):raise ImportErrorSafe('Incomplete/invalid movie: '+str(rel))
    for active in (root/'media').glob('*/active.txt'):
        label=active.parent.name
        lines=active.read_text(encoding='utf-8').splitlines()
        if label not in {'en','ja'} or len(lines)!=3 or lines[0]!='REKHCOM_MEDIA_ACTIVE_V1':raise ImportErrorSafe('Invalid variant index.')
        for component,line in zip(('movies','scenes'),lines[1:]):
            fields=line.split()
            if len(fields)!=2 or fields[0]!=component:raise ImportErrorSafe('Invalid variant component.')
            generation=fields[1]
            if generation=='-':continue
            if not re.fullmatch(r'g-[a-z0-9-]{1,61}',generation):raise ImportErrorSafe('Unsafe variant generation.')
            base=active.parent/generation
            records=(base/'media.meta').read_text(encoding='utf-8').splitlines()
            header=records[0].split() if records else []
            if len(header)!=8 or header[:3]!=['REKHCOM_MEDIA_V1',label,'user']:raise ImportErrorSafe('Invalid variant provenance; no language is inferred.')
            if not re.fullmatch(r'[a-z0-9]{1,63}',header[3]):raise ImportErrorSafe('Unsafe source fingerprint.')
            if header[4] not in {'0','1','2'} or header[5] not in {'0','1'} or header[6] not in {'0','1'} or header[5 if component=='movies' else 6]!='1':raise ImportErrorSafe('Unavailable variant component.')
            if not header[-1].isdigit() or not 1<=int(header[-1])<=256 or int(header[-1])!=len(records)-1 or len(records)>257:raise ImportErrorSafe('Incomplete variant manifest.')
            previous="";has_component=False
            for record in records[1:]:
                row=record.split()
                if len(row)!=2 or not re.fullmatch(r'(movies|scenes)/[A-Za-z0-9_]{1,80}\.mp4',row[0]) or not row[1].isdigit():raise ImportErrorSafe('Unsafe variant member.')
                if row[0]<=previous:raise ImportErrorSafe('Variant members must be unique and sorted.')
                previous=row[0];has_component=has_component or row[0].startswith(component+'/')
                if (base/row[0]).stat().st_size!=int(row[1]):raise ImportErrorSafe('Incomplete variant file.')
            if not has_component:raise ImportErrorSafe('Variant component has no files.')
    return files

def copy_tree(source,target,cancel):
    for path,rel in scan(source):
        if cancel():raise ImportErrorSafe('Import canceled; previous HD preserved.')
        output=target/rel;output.parent.mkdir(parents=True,exist_ok=True)
        with path.open('rb') as inp,output.open('wb') as out:
            while True:
                if cancel():raise ImportErrorSafe('Import canceled; previous HD preserved.')
                block=inp.read(1024*1024)
                if not block:break
                out.write(block)

def transaction(data,source,producer=None,cancel=lambda:False):
    data=Path(data).resolve();raw=Path(source)
    if raw.is_symlink():raise ImportErrorSafe('Select a real source directory, not a symbolic link.')
    source=raw.resolve();target=data/'hd'
    if not source.is_dir():raise ImportErrorSafe('Source folder does not exist.')
    if source==data or source==target or source.is_relative_to(target) or target.is_relative_to(source):raise ImportErrorSafe('Source and game HD destination must be separate folders.')
    if target.is_symlink() or (target.exists() and not target.is_dir()):raise ImportErrorSafe('HD destination must be a real directory.')
    if producer is None:validate(source) # reject incomplete incoming variants BEFORE merging old generations
    if (target/'import.journal').exists():raise ImportErrorSafe('Finish/discard the existing desktop import first; its journal is preserved.')
    lock=data/'.rekhcom-hd-import.lock';stage=None;backup=None
    try:fd=os.open(str(lock),os.O_CREAT|os.O_EXCL|os.O_WRONLY,0o600)
    except FileExistsError:raise ImportErrorSafe('Another import/unfinished lock exists; no files changed.')
    os.close(fd)
    try:
        stage=Path(tempfile.mkdtemp(prefix='.hd-import-',dir=data));new=stage/'hd';new.mkdir()
        if target.exists():copy_tree(target,new,cancel)
        if producer:producer(source,stage)
        else:copy_tree(source,new,cancel)
        validate(new)
        if cancel():raise ImportErrorSafe('Import canceled; previous HD preserved.')
        try:
            if target.exists():
                backup=data/('hd.previous-'+uuid.uuid4().hex)
                os.replace(target,backup)
            os.replace(new,target)
        except BaseException:
            if backup and not target.exists():os.replace(backup,target)
            raise
        return backup
    finally:
        if stage and stage.parent==data and stage.name.startswith('.hd-import-'):shutil.rmtree(stage,ignore_errors=True)
        lock.unlink(missing_ok=True)

def references():
    here=Path(__file__).resolve().parent
    for path in (here/'port/tools',here.parents[1]/'tools'):
        if (path/'khhd_import.py').is_file():return path
    raise ImportErrorSafe('Packaged reference importer is missing.')

def source_producer(sprites=False):
    if sprites and (importlib.util.find_spec('numpy') is None or importlib.util.find_spec('PIL') is None):
        raise ImportErrorSafe('Optional sprite import needs Python numpy and Pillow. Nothing was installed; prepared HD import needs neither.')
    tools=references()
    def run(source,stage):
        if sprites:
            script=tools/'khhd_sprites.py';args=[str(source/'Image/dt/Recom.hed'),str(stage)]
        else:script=tools/'khhd_import.py';args=[str(source),str(stage),'--scenes']
        process=subprocess.Popen([sys.executable,str(script),*args])
        try:code=process.wait()
        except BaseException:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
            raise
        if code:raise subprocess.CalledProcessError(code,process.args)
    return run

def dialog(tool,kind,text):
    if tool=='kdialog':
        args=['kdialog','--getexistingdirectory',str(Path.home()),'--title',text] if kind=='directory' else ['kdialog','--msgbox',text]
    else:args=['zenity','--file-selection','--directory','--title',text] if kind=='directory' else ['zenity','--info','--no-markup','--text',text]
    result=subprocess.run(args,text=True,stdout=subprocess.PIPE)
    return result.stdout.strip() if result.returncode==0 else ''

def migrate_runtime_preference(data):
    """Use the common selector before loading/showing any architecture controls."""
    script=data/'ReKHCoM.sh'
    if script.is_file():
        subprocess.run(['/bin/sh',str(script),'--migrate-runtime-only'],check=False)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--data',type=Path,required=True)
    group=p.add_mutually_exclusive_group()
    for name in ('import-hd','import-source','import-sprites'):group.add_argument('--'+name,type=Path)
    group.add_argument('--play',action='store_true');group.add_argument('--gui',action='store_true')
    p.add_argument('--terminal',action='store_true',help=argparse.SUPPRESS)
    runtime=p.add_mutually_exclusive_group()
    for option in ('force32','force64','auto'):
        runtime.add_argument('--'+option,dest='runtime_override',action='store_const',const='--'+option)
    a=p.parse_args();action='play' if a.play else 'prepared' if a.import_hd else 'source' if a.import_source else 'sprites' if a.import_sprites else ''
    migrate_runtime_preference(a.data)
    source=a.import_hd or a.import_source or a.import_sprites
    tool='kdialog' if shutil.which('kdialog') else 'zenity' if shutil.which('zenity') else None
    if not action:
        if not tool or not (os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY')):
            p.print_help();print('No desktop dialog available. Use the CLI options above; Play directly with ReKHCoM.sh.');return 0
        if tool=='kdialog':args=['kdialog','--menu','Basic native launcher: close the game before importing. Progress/cancel: this terminal.', 'play','Play','prepared','Import prepared HD folder','source','Import owned PC music/movies','sprites','Import owned PC sprites (optional numpy/Pillow)']
        else:args=['zenity','--list','--title','ReKHCoM native launcher','--column','Action','play','prepared','source','sprites']
        result=subprocess.run(args,text=True,stdout=subprocess.PIPE)
        if result.returncode:return 0
        action=result.stdout.strip()
        if action!='play':source=Path(dialog(tool,'directory','Select completed HD folder' if action=='prepared' else 'Select owned PC 1.5+2.5 install'))
        if action!='play' and str(source)=='.':return 0
        if action!='play':
            option={'prepared':'--import-hd','source':'--import-source','sprites':'--import-sprites'}[action]
            command=[sys.executable,str(Path(__file__).resolve()),'--data',str(a.data.resolve()),option,str(source),'--terminal']
            terminal=next((name for name in ('konsole','xterm','gnome-terminal','x-terminal-emulator') if shutil.which(name)),None)
            if not terminal:
                dialog(tool,'info','Import needs a visible terminal for progress and Ctrl+C cancellation. No import started. Run ReKHCoM Launcher.sh --help in your terminal.')
                return 2
            prefix=[terminal,'--nofork','-e'] if terminal=='konsole' else [terminal,'--wait','--'] if terminal=='gnome-terminal' else [terminal,'-e']
            return subprocess.run(prefix+command).returncode
    if action=='play':os.execv('/bin/sh',['/bin/sh',str(a.data/'ReKHCoM.sh'),*([a.runtime_override] if a.runtime_override else [])])
    try:
        if action=='prepared' and (source/'hd').is_dir():source=source/'hd'
        backup=transaction(a.data,source,None if action=='prepared' else source_producer(action=='sprites'))
        message='HD import complete. Legacy imports remain language unknown.'+(' Previous HD retained at '+str(backup) if backup else '')
        print(message)
        if tool and a.gui:dialog(tool,'info',message)
        return 0
    except (Exception,KeyboardInterrupt) as error:
        message='Import stopped; previous HD preserved. '+str(error);print(message,file=sys.stderr)
        if tool and a.gui:dialog(tool,'info',message)
        return 1
if __name__=='__main__':
    code=main()
    if '--terminal' in sys.argv and sys.stdin.isatty():
        try:input('Press Enter to close this import terminal...')
        except (EOFError,KeyboardInterrupt):pass
    raise SystemExit(code)
