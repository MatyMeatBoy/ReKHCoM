"""Sign a handheld's card in to RetroAchievements from a PC with internet.

usage: python ra_link_card.py [SD root, default F:]

Reads <SD>/ports/rekhcomga36/retroachievements_cuenta.txt (usuario= / contrasena=), signs in once
(the same request rcheevos sends), stores the session token where the game reads it
(retroachievements.auth, 128-byte user + 128-byte token) and enables the account
(retroachievements.ini). The password is then removed from the card's file. Neither the
password nor the token is ever printed.
"""
import json
import sys
import urllib.parse
import urllib.request
from pathlib import Path

root = Path(sys.argv[1] if len(sys.argv) > 1 else 'F:/')
game = root / 'ports' / 'rekhcomga36'
account = game / 'retroachievements_cuenta.txt'
if not account.is_file():
    raise SystemExit(f'No encuentro {account}. Pon la SD y crea el archivo con usuario= y contrasena=.')

user = password = ''
for line in account.read_text(encoding='utf-8', errors='replace').splitlines():
    if line.startswith('#') or '=' not in line:
        continue
    key, value = line.split('=', 1)
    key = key.strip().lower()
    if key in ('usuario', 'user', 'username'):
        user = value.strip()
    elif key in ('contrasena', 'contraseña', 'password'):
        password = value.rstrip('\r\n')
if not user or not password:
    raise SystemExit('El archivo necesita una linea usuario= y otra contrasena=.')

body = urllib.parse.urlencode({'r': 'login2', 'u': user, 'p': password}).encode()
password = None
request = urllib.request.Request('https://retroachievements.org/dorequest.php', data=body, method='POST',
                                 headers={'Content-Type': 'application/x-www-form-urlencoded',
                                          'User-Agent': 'ReKHCoM/2.10 rcheevos/12.5'})
body = None
try:
    with urllib.request.urlopen(request, timeout=20) as response:
        reply = json.loads(response.read().decode('utf-8', errors='replace'))
except Exception as error:  # network or server: nothing on the card is changed
    raise SystemExit(f'No se pudo conectar con RetroAchievements ({type(error).__name__}). La tarjeta no se modifico.')
request = None
if not reply.get('Success') or not reply.get('Token'):
    raise SystemExit('RetroAchievements rechazo el inicio de sesion: ' + str(reply.get('Error', 'sin detalle')) +
                     '. La tarjeta no se modifico.')

name = str(reply.get('User') or user)
token = str(reply['Token'])
if len(name.encode()) >= 128 or len(token.encode()) >= 128:
    raise SystemExit('Respuesta inesperada del servidor. La tarjeta no se modifico.')
auth = name.encode().ljust(128, b'\0') + token.encode().ljust(128, b'\0')
token = None
(game / 'retroachievements.auth').write_bytes(auth)
auth = None
(game / 'retroachievements.ini').write_text('enabled=1\n', newline='\n')
account.write_text(f'usuario={name}\n# Sesion iniciada desde el PC: la contrasena se borro de este archivo.\n',
                   encoding='utf-8', newline='\n')
print(f'Listo: la tarjeta queda vinculada a la cuenta {name}.')
print('La contrasena se borro del archivo. Ya puedes poner la SD en la consola.')
