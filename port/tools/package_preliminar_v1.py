"""Package completed Preliminar V1 entries into a private playable review copy."""
import argparse,json,shutil
from pathlib import Path
from preliminar_v1_recipes import READY,ROOT
from test_preliminar_v1 import fixtures,WINDOWS

def package(build,out,evidence):
 selected=[c for c in fixtures() if c.get('scene') in READY]
 for c in selected:
  report=json.loads((evidence/c['name']/'report.json').read_text())
  if not report['passed'] or len(report.get('checks',[]))<12:raise RuntimeError('incomplete fixture: '+c['name'])
 out.mkdir(parents=True,exist_ok=True)
 for n in ('khcom.exe','SDL3.dll'):shutil.copy2(ROOT/build/n,out/n)
 for n in ('assets',):
  if (ROOT/n).exists():shutil.copytree(ROOT/n,out/n,dirs_exist_ok=True)
 (out/'roms').mkdir(exist_ok=True);shutil.copy2(ROOT/'roms/B8CP.gba',out/'roms/B8CP.gba')
 (out/'user-data').mkdir(exist_ok=True)
 (out/'user-data/kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nhd_text=0\nfullscreen=0\nvsync=1\n')
 menu=['@echo off','setlocal','for /f "tokens=1 delims==" %%V in (\'set KHCOM_ 2^>nul\') do set "%%V="','echo Preliminar V1 - piezas terminadas para comentar']
 labels={97:'097 - Bosque de 100 Acre',95:'095 - Patio del laberinto de Wonderland',90:'090 - Cementerio de Halloween Town',87:'087 - Sala oscura de Halloween Town',81:'081 - Pasillo de Monstro E3',7:'007 - Patio de Ciudad de Paso',44:'044 - Plaza al atardecer',45:'045 - Entrada al bosque del castillo',53:'053 - Playa de Isla del Destino',54:'054 - Playa de Isla del Destino (Riku)',55:'055 - Isla del Destino se desmorona'}
 shared={
  'evento-088-escena-087':'088 - Halloween Town (E0)',
  'evento-082-escena-081':'082 - Monstro E3 (Victoria)',
  'evento-083-escena-081':'083 - Monstro E3 (Fallo 1)',
  'evento-084-escena-081':'084 - Monstro E3 (Fallo 2)',
  'evento-085-escena-081':'085 - Monstro E3 (Reintento)',
  'evento-177-escena-053':'177 - Playa de Isla del Destino (Riku)',
  'evento-178-escena-054':'178 - Playa de Isla del Destino (Riku)',
  'evento-056-escena-055':'056 - Isla del Destino (Sora)',
  'evento-179-escena-055':'179 - Isla del Destino (Riku)',
  'evento-181-escena-055':'181 - Isla del Destino (Riku)',
  'evento-183-escena-055':'183 - Isla del Destino (Riku)',
 }
 for i,c in enumerate(selected,1):
  label=shared.get(c['name'],labels.get(c.get('scene'),c['name']))
  menu.append(f"echo {i}. {label}")
 menu+=['set "pieza=1"','set /p "pieza=Escena [1]: "']
 for i,c in enumerate(selected,1):
  menu.append(f'if "%pieza%"=="{i}" set "{c["start"][0]}={c["start"][1]}"')
  menu.append(f'if "%pieza%"=="{i}" set "escena={c["scene"]}"')
 menu+=['if not defined KHCOM_EVENT exit /b','echo 1. 1920x1080','echo 2. 2560x1080','echo 3. 3440x1440','echo 4. 3840x1080','set "ancho=1"','set /p "ancho=Ventana [1]: "','set "KHCOM_WINDOW=1920x1080"']
 for i,(w,h) in enumerate(WINDOWS[1:],2):menu.append(f'if "%ancho%"=="{i}" set "KHCOM_WINDOW={w}x{h}"')
 menu+=['set "KHCOM_PRELIMINAR_V1=1"','set "KHCOM_AGRABAH_WIDE=1"','set "KHCOM_AGRABAH109_WIDE=1"','set "antes=N"','set /p "antes=Ver antes del arreglo? [S/N, N]: "','if /i "%antes%"=="S" set "KHCOM_PRELIMINAR_SKIP=%escena%"','set "KHCOM_KEYS=60-63:A"','cd /d "%~dp0"','khcom.exe --language es --rom "roms/B8CP.gba" --data-dir "user-data"','endlocal']
 (out/'Probar Preliminar V1.bat').write_bytes(('\r\n'.join(menu)+'\r\n').encode('ascii'))
 for c in selected:
  source=evidence/c['name'];dest=out/'revision'/c['name'];dest.mkdir(parents=True,exist_ok=True)
  report=json.loads((source/'report.json').read_text())
  if not report['passed'] or len(report.get('checks',[]))<12:raise RuntimeError('incomplete fixture: '+c['name'])
  for p in source.glob('*.png'):shutil.copy2(p,dest/p.name)
  shutil.copy2(source/'report.json',dest/'report.json')
 (out/'LEEME.txt').write_text('Preliminar V1: versiones implementadas pendientes de comentarios del usuario.\nEjecutar Probar Preliminar V1.bat. Escala x3, cuatro ventanas y opcion antes/despues.\nCodex hace commits; Claude integra. No se ha integrado ni publicado.\nPiezas disponibles: '+', '.join(c['name'] for c in selected)+'\n',encoding='utf-8')
 print(out)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--build',default='build/agrabah109-eu');p.add_argument('--out',default='../ReKHCoM-preliminarV1');p.add_argument('--evidence',default='build/preliminar-v1/ready');a=p.parse_args();package(Path(a.build),(ROOT/a.out).resolve(),ROOT/a.evidence)
