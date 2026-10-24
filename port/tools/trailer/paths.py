"""Shared paths for the optional Windows trailer workflow (private ROMs required)."""
import os,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
T=Path(os.environ.get('KHCOM_TRAILER_DIR',ROOT.parent/'arte/Trailer/v182')).resolve()
FF=shutil.which('ffmpeg')
if not FF:
 candidates=sorted((Path(os.environ['LOCALAPPDATA'])/'Microsoft/WinGet/Packages').glob('Gyan.FFmpeg_*/ffmpeg-*/bin/ffmpeg.exe'))
 if not candidates:raise RuntimeError('Install ffmpeg or add it to PATH')
 FF=str(candidates[-1])
SAVE_SOURCE=Path(os.environ['KHCOM_TRAILER_SAVE']) if os.environ.get('KHCOM_TRAILER_SAVE') else None
for path in [T/'clips',T/'audio/sfx',T/'edit',T/'tools/cards',T/'previews']:path.mkdir(parents=True,exist_ok=True)
