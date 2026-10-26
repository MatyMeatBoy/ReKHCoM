"""Compile the launcher's actual account selector; test only fake Steam folders.

No Steam process or real user configuration is changed.
"""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    out = ROOT / 'build/steam-account-test'
    out.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'port/decompui/decompui.cpp').read_text(encoding='utf-8')
    begin = source.index('static std::vector<fs::path> SteamUserConfigs(')
    end = source.index('static fs::path SteamFolder()', begin)
    unit = '#include <filesystem>\n#include <vector>\n#include <string>\n#include <fstream>\n#include <cctype>\n#include <cstdlib>\n#include <iostream>\nnamespace fs=std::filesystem;\n'
    unit += source[begin:end]
    unit += '\nint main(int argc,char** argv){for(auto& p:SteamUserConfigs(fs::u8path(argv[1]))) std::cout<<p.parent_path().filename().string()<<"\\n";}\n'
    (out / 'selector.cpp').write_text(unit, encoding='utf-8')
    compiler = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang++.exe'
    subprocess.run([str(compiler), '-std=c++17', '-O0', '-static', str(out / 'selector.cpp'), '-o', str(out / 'selector.exe')], check=True)
    base = 76561197960265728
    cases = [
        ('most_recent', [(123, 999, 0), (456, 100, 1), (789, 1200, 0)], {'456'}),
        ('timestamp', [(123, 999, 0), (456, 100, 0), (789, 1200, 0)], {'789'}),
        ('marker_before_timestamp', [(456, 100, 1), (789, 1200, 0)], {'456'}),
        ('missing_loginusers', [], {'123', '456', '789'}),
    ]
    for name, accounts, expected in cases:
        steam = out / name
        (steam / 'config').mkdir(parents=True, exist_ok=True)
        for account in (0, 123, 456, 789):
            (steam / 'userdata' / str(account) / 'config').mkdir(parents=True, exist_ok=True)
        if accounts:
            text = '"users"\n{\n' + ''.join(
                f'"{base + account}"\n{{\n"AccountName" "fake"\n"MostRecent" "{recent}"\n"timestamp" "{timestamp}"\n}}\n'
                for account, timestamp, recent in accounts) + '}\n'
            (steam / 'config/loginusers.vdf').write_text(text, encoding='utf-8')
        result = subprocess.run([str(out / 'selector.exe'), str(steam)], check=True, capture_output=True, text=True,
                                creationflags=getattr(subprocess, 'BELOW_NORMAL_PRIORITY_CLASS', 0))
        got = set(result.stdout.splitlines())
        assert got == expected, (name, got, expected)
        print(f'PASS {name}')
    if os.name == 'nt':
        import winreg
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam') as key:
                steam = Path(winreg.QueryValueEx(key, 'SteamPath')[0])
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam\ActiveProcess') as key:
                active = winreg.QueryValueEx(key, 'ActiveUser')[0]
            result = subprocess.run([str(out / 'selector.exe'), str(steam)], check=True, capture_output=True, text=True)
            print('Real Steam read-only: selects active account =', set(result.stdout.splitlines()) == {str(active)})
        except OSError:
            print('Real Steam read-only: registry unavailable')


if __name__ == '__main__':
    main()
