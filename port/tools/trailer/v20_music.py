"""Clean native title phrase and imported HD phrase, matched to RMS -20 dBFS.

Requires NumPy; never plays audio through the device. Capture the title after
boot and explicitly restart song 6 at frame 900, avoiding copyright silence.
"""
import json,subprocess,wave
import numpy as np
from v20_capture import capture,T,FF,HD

if __name__=='__main__':
    capture(dict(name='music_title_gba_clean',a=900,b=2100,hd=False,size='640x360',keys='',ini='music_volume=100\nsfx_volume=0\n',extra={'KHCOM_MODE':'10:mode_title','KHCOM_SONG':'900:6'}))
    metadata=[]
    for label,source in [('gba',T/'audio/music_title_gba_clean.wav'),('hd',HD/'music/Title.ogg')]:
        p=subprocess.run([FF,'-v','error','-i',str(source),'-t','20','-f','f32le','-ac','2','-ar','48000','-'],capture_output=True,check=True,creationflags=0x08000000)
        pcm=np.frombuffer(p.stdout,dtype='<f4');rms=float(np.sqrt(np.mean(pcm.astype(np.float64)**2)));gain=0.1/rms
        assert np.max(np.abs(pcm))*gain<1,'matched phrase would clip'
        out=T/'audio'/f'music_title_{label}_matched.wav'
        with wave.open(str(out),'wb') as w:
            w.setnchannels(2);w.setsampwidth(2);w.setframerate(48000);w.writeframes((pcm*gain*32767).astype('<i2').tobytes())
        metadata.append(dict(label=label,source=str(source),source_start=0,source_rms=rms,source_peak=float(np.max(np.abs(pcm))),output=str(out),gain=gain,output_rms=0.1))
    (T/'audio/music_title_match.json').write_text(json.dumps(metadata,indent=2))
