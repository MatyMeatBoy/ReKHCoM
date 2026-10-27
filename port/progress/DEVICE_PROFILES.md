# Perfiles de dispositivo compartidos

`port_device_profile.h` contiene el clasificador puro Generic/SteamDeck. Sólo Linux
(de escritorio) y Windows admiten SteamDeck. `SteamDeck=1` exacto es una señal del
entorno Steam; Linux también reconoce DMI `sys_vendor=Valve` junto con
`product_name=Jupiter` o `Galileo`. Ausencia, valores parciales y otros fabricantes
con esos nombres no identifican Deck. Android y plataformas desconocidas conservan
Generic. Windows consulta exclusivamente `SystemManufacturer` y `SystemProductName`
en `HKLM HARDWARE\DESCRIPTION\System\BIOS` mediante `RegGetValueW` cargado de
advapi32 din�micamente: sin WMI ni nueva dependencia de enlace. Valida tipo REG_SZ,
longitud UTF-16, terminaci�n y ASCII exacto antes de clasificar Valve+Jupiter/Galileo.
Error, valor ausente o malformado conserva Generic. La se�al Steam tiene prioridad.

El adaptador nativo `PortDeviceHostQuery` responde la consulta reservada
`KHCOM_HOST_DEVICE_PROFILE`. Los hosts de bridge deben interceptarla antes de leer
el entorno; SDL_getenv del guest la transporta por el bridge existente. No es una
opción de usuario ni un override de entorno. Sin contexto de host, el guest usa
Generic, incluso si su compilador define Linux.

El core aplica fullscreen=1 una sola vez antes de leer cualquier ini. Auto ya es el
layout de mando inicial y se conserva. No cambia gráficos ni habilita HD. Las claves
presentes en `kh_com.ini` (o el preset heredado `kh_com.default.ini` cuando no existe
el primero) prevalecen; también prevalecen cambios posteriores de Options. Un único
log de inicio incluye perfil y motivo. El paquete Linux común debe dejar los
ajustes específicos de Deck al core y no distribuir un ini exclusivo de Deck.

Prueba pequeña: `python port/tools/test_device_profiles.py`. Compila el clasificador
y los bloques reales del core de defaults y parser ini, sin compilar el juego.
