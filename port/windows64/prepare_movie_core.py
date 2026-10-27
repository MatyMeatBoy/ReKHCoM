"""Reuse the native Windows decoder and subtitle implementation without game-pointer dependencies.
Generated includes stay in build/windows64, never in the source tree.
Fail on changed source anchors rather than silently compiling a stale fork.
"""
from pathlib import Path

def prepare(root, out):
    source = (root / 'port/src/port_hdmovie.c').read_text(encoding='utf-8')
    start = source.index('#define SUB_MAX 256')
    stop = source.index('/* Plays hd/movies/', start)
    subtitles = source[start:stop]
    old = 'MovieSubtitlePath(name, order[i], path, sizeof(path));'
    assert subtitles.count(old) == 1, 'native subtitle path contract changed'
    subtitles = subtitles.replace(old, 'MovieSubtitlePath(name, i, path, sizeof(path));')
    # The bridge supplies paths already resolved for the current guest language.
    subtitles = subtitles.replace('    SubsOrder(order);', '    (void)order;')
    start = source.index('static int PlayFile(const char* name) {', stop)
    stop = source.index('\n#else\n/* The path in single quotes', start)
    decoder = source[start:stop]
    old = '    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)\n        return 0;\n    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 1024);'
    assert decoder.count(old) == 1, 'native movie path contract changed'
    decoder = decoder.replace(old, '    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wpath, 1024) ||\n        GetFileAttributesW(wpath) == INVALID_FILE_ATTRIBUTES) return 0;')
    # Reset cancellation and its false result remain the native decoder's policy.
    assert 'return played && !sMovieDeviceReset;' in decoder, 'native reset outcome contract changed'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'windows_movie_subtitles.inc').write_text(subtitles, encoding='utf-8')
    (out / 'windows_movie_decoder.inc').write_text(decoder, encoding='utf-8')
    # Compile the production event consumer in the host regression, not a mirrored implementation.
    input_source = (root / 'port/src/port_input.c').read_text(encoding='utf-8')
    start = input_source.index('void PortInputHandleEvent(const void* p) {')
    stop = input_source.index('\nint PortInputKeyBound(', start)
    (out / 'windows_input_event_test.inc').write_text(input_source[start:stop], encoding='utf-8')
