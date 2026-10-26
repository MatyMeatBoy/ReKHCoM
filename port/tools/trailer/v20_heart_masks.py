from pathlib import Path
import subprocess,numpy as np,json,time
from v20_capture import T,FF
out=T/'heart-masks';out.mkdir(exist_ok=True,parents=True)
W,H=1920,1080
x=(np.arange(W,dtype=np.float64)-W/2)[None,:];y=(H*.47-np.arange(H,dtype=np.float64))[:,None]
base=x*x+y*y;term=(x*x)*(y*y*y)
meta={'width':W,'height':H,'fps':60,'geometry':'same implicit heart, d>0 algebra times d^6','sampling':'P=float32(1-n/N), exact xfade duration step; next frame at N is B','files':[],'source':'https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/vf_xfade.c'}
t0=time.perf_counter()
for N in [48,42]:
 procs={};files={}
 for name in ['inner','outer']:
  f=out/f'heart_{N}_{name}_444.mkv';files[name]=f
  cmd=[FF,'-v','error','-f','rawvideo','-pix_fmt','yuv444p','-s',f'{W}x{H}','-r','60','-i','-','-an','-c:v','ffv1','-level','3','-g','1','-threads','2','-pix_fmt','yuv444p','-y',str(f)]
  procs[name]=subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=0x08000000)
 for n in range(N):
  P=float(np.float32(1-n/N))
  for name,k in [('inner',1.75),('outer',1.85)]:
   d=H*(k*(1-P)**1.5+.0001);q=base-d*d;m=((q*q*q-term*d)<=0).astype(np.uint8)*255
   raw=m.tobytes();procs[name].stdin.write(raw);procs[name].stdin.write(raw);procs[name].stdin.write(raw)
 for name,proc in procs.items():
  proc.stdin.close();err=proc.stderr.read().decode();ret=proc.wait();assert ret==0,(ret,err)
  f=files[name];f420=out/f'heart_{N}_{name}_420.mkv'
  subprocess.run([FF,'-v','error','-i',str(f),'-vf','scale=flags=neighbor','-pix_fmt','yuv420p','-c:v','ffv1','-level','3','-g','1','-threads','2','-y',str(f420)],check=True)
  meta['files'].append({'frames':N,'mask':name,'file444':str(f),'file420':str(f420),'bytes444':f.stat().st_size})
(out/'manifest.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
print('Masks ready',round(time.perf_counter()-t0,2),'seconds');print(json.dumps(meta['files'],indent=2))
