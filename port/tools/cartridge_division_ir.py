"""Lower scalar game i32 DIV/REM to the verified cartridge helper contract.

Run on frontend LLVM IR BEFORE optimization: C division by zero and signed
MIN/-1 otherwise become poison or target traps before a late guard can help.
This pass is intentionally not applied to the host, SDL or imported content.
"""
import hashlib
from pathlib import Path
import re

PREFIX='__rekhcom_gba_'
OPS=('sdiv','srem','udiv','urem')
OPERAND=r'(?:%[-\w.$]+|-?\d+|undef|poison)'
LINE=re.compile(r'^(\s*)(%[-\w.$]+)\s*=\s*(sdiv|srem|udiv|urem)(?: exact)? i32 ('+OPERAND+r'), ('+OPERAND+r')(\s*(?:,.*)?)$')

def fingerprint():
    return hashlib.sha256(Path(__file__).read_bytes()).hexdigest()

def helper(op):
    name=PREFIX+op+'32'
    # Branches protect the operation itself, not just the observed result.
    body=f'''define internal i32 @{name}(i32 %n, i32 %d) alwaysinline nounwind {{
entry:
  %zero = icmp eq i32 %d, 0
  br i1 %zero, label %zero_result, label %nonzero
zero_result:
  ret i32 0
nonzero:
'''
    if op.startswith('s'):
        body+='''  %minimum = icmp eq i32 %n, -2147483648
  %minus_one = icmp eq i32 %d, -1
  %overflow = and i1 %minimum, %minus_one
  br i1 %overflow, label %overflow_result, label %normal
overflow_result:
  ret i32 '''+('-2147483648' if op=='sdiv' else '0')+'''
normal:
'''
    body+=f'  %result = {op} i32 %n, %d\n  ret i32 %result\n}}\n'
    return body

def lower_cartridge_division(source):
    if '@'+PREFIX in source:
        raise ValueError('Reserved cartridge arithmetic helper namespace already present')
    used=set();lines=[]
    for line in source.splitlines(keepends=True):
        match=LINE.fullmatch(line.rstrip('\r\n'))
        if match:
            indent,dest,op,a,b,metadata=match.groups()
            used.add(op)
            ending='\n' if line.endswith('\n') else ''
            lines.append(f'{indent}{dest} = call i32 @{PREFIX}{op}32(i32 {a}, i32 {b}){metadata}{ending}')
        else:
            if re.search(r'=\s*(?:sdiv|srem|udiv|urem)\b[^\n]*\bi32\b',line) and not re.search(r'<[^>]+>',line):
                raise ValueError('Unsupported scalar i32 division form: '+line.strip())
            lines.append(line)
    if not used:return source
    return ''.join(lines)+'\n; Cartridge scalar i32 arithmetic\n'+''.join(helper(op) for op in OPS if op in used)
