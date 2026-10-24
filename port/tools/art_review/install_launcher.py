"""Install/update the one user-facing BAT at an explicitly selected project root."""
import argparse
import os
from pathlib import Path


def install(root):
    root=Path(root).resolve()
    if not root.is_dir():raise ValueError('La raíz debe existir')
    target=root/'Revisar preliminares.bat'
    if target.exists():
        old=target.read_text(encoding='utf-8')
        if 'art_review\\launch.ps1' not in old and 'ReKHCoM unified art review' not in old:
            raise ValueError('El BAT existente no pertenece a esta herramienta; no se reemplazó')
    relative=os.path.relpath(Path(__file__).with_name('launch.ps1'),root)
    target.write_text('@echo off\nrem ReKHCoM unified art review launcher\npowershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0'+relative+'"\nif errorlevel 1 pause\n',encoding='utf-8',newline='\r\n')
    return target


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True)
    print(install(p.parse_args().root))
