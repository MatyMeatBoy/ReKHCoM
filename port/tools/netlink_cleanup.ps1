# Stops the hidden test games a broken netlink_pair.sh / netlink_android.sh run left behind (they hold the ports).
Get-CimInstance Win32_Process -Filter "Name='khcom.exe'" |
    Where-Object { $_.ExecutablePath -match '\\build\\netlink\\' } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force }
