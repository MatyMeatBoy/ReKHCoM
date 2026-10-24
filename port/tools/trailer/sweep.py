import subprocess
from paths import FF,T
CL=str(T/'clips')
# boundary position (fraction of width) over q=1-P in [0,1]: ease to 0.5, hold, ease to 0
def ease(x): return f"(0.5-0.5*cos(PI*{x}))"
q="(1-P)"
b=(f"if(lt({q},0.35),1-0.5*{ease(f'({q}/0.35)')},"
   f"if(lt({q},0.72),0.5,0.5-0.5*{ease(f'(({q}-0.72)/0.28)')}))")
expr=f"if(lt(abs(X-W*{b}),W*0.0016),if(eq(PLANE,0),250,128),if(gt(X,W*{b}),B,A))"
F=r"C\:/Windows/Fonts/pala.ttf"
# labels follow the halves: ORIGINAL TEXT fades out at the end, HD TEXT in
lab=(f"drawbox=x=0:y=0:w=iw:h=110:color=black@0.55:t=fill,"
     f"drawtext=fontfile='{F}':text='ORIGINAL TEXT':fontsize=58:fontcolor=white:x=80:y=26:alpha='if(lt(t,0.3),t/0.3,if(gt(t,4.6),max(0,(5.0-t)/0.4),1))',"
     f"drawtext=fontfile='{F}':text='HD TEXT':fontsize=58:fontcolor=white:x=w-tw-80:y=26:alpha='if(lt(t,0.9),0,if(lt(t,1.3),(t-0.9)/0.4,1))'")
D=5.4
g=(f"[0:v]setpts=PTS-STARTPTS,tpad=stop_mode=clone:stop_duration=4[a];[1:v]setpts=PTS-STARTPTS,tpad=stop_mode=clone:stop_duration=4[b];"
   f"[a]format=yuv420p[a2];[b]format=yuv420p[b2];[a2][b2]xfade=transition=custom:duration=4.4:offset=0.5:expr='{expr}',trim=duration={D},scale=2688:1512:flags=neighbor,crop=1920:1080:(iw-1920)/2:ih-1080-28,{lab},format=yuv422p10le[v]")
r=subprocess.run([FF,'-hide_banner','-loglevel','error','-y','-i',CL+r'\09a_tienda_texto_pixel.mov','-i',CL+r'\09b_tienda_texto_hd.mov','-filter_complex',g,'-map','[v]','-c:v','prores_ks','-profile:v','3','-threads','2',CL+r'\09c_texto_barrido.mov'],capture_output=True,text=True,creationflags=0x08000000)
print(r.returncode, r.stderr[-1500:])
