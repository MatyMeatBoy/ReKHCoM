"""Read only the recorder child PID's native reload state; never write memory.

Uses the exact executable's fixed PE symbols, as the KHST observer does.
The last observed state of each frame is recorded. Real-time pacing gives
the 1 ms sampler ample time without adding a save-state notification.
"""
import ctypes,json,struct,sys,threading,time
from pathlib import Path

def start_reload_trace(process,executable,destination):
    sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
    from test_gameplay_observe_state import PE
    pe=PE(executable);frame_address=pe.symbols['_sFrameCount'];card_address=pe.symbols['_gCardBattleState']
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.OpenProcess.argtypes=[ctypes.c_uint32,ctypes.c_int,ctypes.c_uint32];kernel.OpenProcess.restype=ctypes.c_void_p
    kernel.ReadProcessMemory.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_size_t)]
    kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    handle=kernel.OpenProcess(0x10|0x1000,False,process.pid)
    if not handle:raise OSError(ctypes.get_last_error(),'Cannot read recorder child process')
    errors=[]
    def read(address,n):
        buf=ctypes.create_string_buffer(n);actual=ctypes.c_size_t()
        if not kernel.ReadProcessMemory(handle,address,buf,n,ctypes.byref(actual)) or actual.value!=n:return None
        return buf.raw
    def work():
        pending=None
        try:
            with Path(destination).open('w',encoding='utf-8') as out:
                while process.poll() is None:
                    f=read(frame_address,4);pointer=read(card_address,4)
                    if f and pointer and (address:=struct.unpack('<I',pointer)[0]):
                        raw=read(address+0xF4,24);f2=read(frame_address,4)
                        if raw and f==f2:
                            frame=struct.unpack('<I',f)[0]
                            sample=dict(frame=frame,gauge=struct.unpack_from('<I',raw)[0],counter=struct.unpack_from('<h',raw,8)[0],full_frame=struct.unpack_from('<h',raw,12)[0],animation=struct.unpack_from('<h',raw,16)[0],full_flags=list(raw[20:24]))
                            if pending and pending['frame']!=frame:out.write(json.dumps(pending)+'\n')
                            pending=sample
                    time.sleep(.001)
                if pending:out.write(json.dumps(pending)+'\n')
        except Exception as exc:errors.append(str(exc))
        finally:kernel.CloseHandle(handle)
    thread=threading.Thread(target=work,daemon=True);thread.start()
    return thread,errors
