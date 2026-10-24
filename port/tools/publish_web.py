"""Validate the static player and optionally publish an isolated gh-pages tree.

No worktree changes or git-add-all. Only explicit web artifacts enter the tree.
"""
import argparse
import datetime
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
FILES = ['index.html', 'web.css', 'web.js', 'runtime.js', 'i18n.js', 'settings.js', 'save-editor.js', 'editor-ui.js', 'achievement-data.js', 'achievements.js', 'cards.js', 'controller.png', 'achievement-badges.png', 'favicon.png', 'apple-touch-icon.png', '.nojekyll', 'LICENSE.md', 'SDL-LICENSE.txt',
         'fonts/KHMenu.otf', 'fonts/ComicHearts.otf', 'fonts/NOTICE.md', 'logo.png', 'boxart.jpg']
FILES += [f'rekhcom-{r}.{ext}' for r in ['eu','us','jp'] for ext in ['js','wasm']]


def git(*args, env=None):
    return subprocess.check_output(['git', *args], cwd=ROOT, env=env, text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdl-source', type=Path, required=True)
    parser.add_argument('--publish', action='store_true')
    args = parser.parse_args()
    site = ROOT / 'build/web'
    (site / '.nojekyll').write_bytes(b'')
    shutil.copy2(ROOT / 'LICENSE.md', site / 'LICENSE.md')
    shutil.copy2(args.sdl_source / 'LICENSE.txt', site / 'SDL-LICENSE.txt')
    for name in ['web.css','web.js','runtime.js','i18n.js','settings.js','save-editor.js','editor-ui.js', 'achievement-data.js', 'achievements.js','cards.js','controller.png','favicon.png','apple-touch-icon.png']:
        shutil.copy2(ROOT / 'port/web' / name, site / name)
    shutil.copy2(ROOT / 'port/assets/achievements/badges.png', site / 'achievement-badges.png')
    # Version the shell too: a cached alpha1 web.js must never drive alpha2.
    html = (ROOT / 'port/web/index.html').read_text(encoding='utf-8')
    for name in ['web.css','runtime.js','i18n.js','web.js','settings.js','save-editor.js','editor-ui.js', 'achievement-data.js', 'achievements.js','cards.js']:
        version = hashlib.sha256((site / name).read_bytes()).hexdigest()[:16]
        html = html.replace(f'"{name}"', f'"{name}?v={version}"')
    (site / 'index.html').write_text(html,encoding='utf-8')
    unexpected = {p.relative_to(site).as_posix() for p in site.rglob('*') if p.is_file()} - set(FILES) - {'manifest.json'}
    if unexpected:
        raise RuntimeError('Unexpected files in web output: ' + ','.join(sorted(unexpected)))
    for region in ['eu','us','jp']:
        wasm = site / f'rekhcom-{region}.wasm'
        if wasm.read_bytes()[:8] != b'\0asm\x01\0\0\0':
            raise RuntimeError('Invalid Wasm binary: ' + str(wasm))
        if f'rekhcom-{region}.wasm' not in (site / f'rekhcom-{region}.js').read_text():
            raise RuntimeError('JS/Wasm filename mismatch')
    release = re.search(r'#define REKHCOM_VERSION\s+"([^"]+)"',
                        (ROOT/'port/include/port/port_version.h').read_text(encoding='utf-8'))[1]
    manifest = {'version':release+'-web-alpha', 'source_commit':git('rev-parse','HEAD'),
                'source_dirty':bool(git('diff','--name-only')),
                'built_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(), 'files':{}}
    for name in FILES:
        data = (site / name).read_bytes()
        manifest['files'][name] = {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    (site / 'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    if not args.publish:
        print(json.dumps(manifest,indent=2))
        return
    if manifest['source_dirty']:
        raise RuntimeError('Commit source changes before publishing')
    # Keep the main index and checkout completely untouched.
    index = ROOT / 'build/web-pages.index'
    env = dict(os.environ, GIT_INDEX_FILE=str(index))
    git('read-tree','--empty',env=env)
    for name in FILES + ['manifest.json']:
        # Preserve the exact bytes covered by manifest hashes on Windows too.
        blob = git('hash-object','--no-filters','-w',str(site / name))
        git('update-index','--add','--cacheinfo',f'100644,{blob},{name}',env=env)
    tree = git('write-tree',env=env)
    remote = git('ls-remote','origin','refs/heads/gh-pages')
    parent = []
    if remote:
        git('fetch','origin','gh-pages')
        parent = ['-p', remote.split()[0]]
    commit = git('commit-tree',tree,*parent,'-m','Publish ReKHCoM browser alpha')
    subprocess.run(['git','push','origin',f'{commit}:refs/heads/gh-pages'],cwd=ROOT,check=True)
    (ROOT / 'build/web-publication.json').write_text(json.dumps(
        {'pages_commit':commit,'manifest':manifest},indent=2),encoding='utf-8')
    print('Published gh-pages commit:',commit)


if __name__ == '__main__':
    main()
