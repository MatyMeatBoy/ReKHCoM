"""The real publisher must refresh cached font bytes and their CSS identity."""
from pathlib import Path
import hashlib,tempfile
import publish_web as publisher

def main():
    assert hasattr(publisher,'stage_font_assets'),'Publisher does not refresh/version source fonts'
    with tempfile.TemporaryDirectory(prefix='web-font-assets-') as directory:
        root=Path(directory);source=root/'source';site=root/'site'
        source.mkdir();(site/'fonts').mkdir(parents=True)
        for name,data in [('KHMenu.otf',b'corrected-font'),('ComicHearts.otf',b'subtitle-font'),('NOTICE.md',b'font-notice')]:
            (source/name).write_bytes(data)
        (site/'fonts/KHMenu.otf').write_bytes(b'cached-old-font')
        css="@font-face{src:url('fonts/KHMenu.otf')} .keep{background:url('other.png')}"
        (site/'web.css').write_text(css,encoding='utf8')
        publisher.stage_font_assets(source,site)
        first=(site/'web.css').read_text(encoding='utf8')
        for name in ('KHMenu.otf','ComicHearts.otf','NOTICE.md'):
            assert (site/'fonts'/name).read_bytes()==(source/name).read_bytes()
        token=hashlib.sha256((source/'KHMenu.otf').read_bytes()).hexdigest()[:16]
        assert "fonts/KHMenu.otf?v="+token in first and "url('other.png')" in first
        publisher.stage_font_assets(source,site)
        assert (site/'web.css').read_text(encoding='utf8')==first,'Repeated preparation changed CSS identity'
        (source/'KHMenu.otf').write_bytes(b'newer-corrected-font')
        publisher.stage_font_assets(source,site)
        assert (site/'web.css').read_text(encoding='utf8')!=first,'Font update kept stale browser URL'
        assert (site/'web.css').read_text(encoding='utf8').count('?v=')==1
    print('PASS cached font refreshed; CSS identity tracks bytes; repeat stable; unrelated URL preserved')

if __name__=='__main__':main()
