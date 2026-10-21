@echo off
setlocal
cd /d "%~dp0"
python port\tools\test_aerith_scene.py
if errorlevel 1 (
    echo.
    echo La prueba no termino correctamente. Revisa los mensajes anteriores.
    pause
    exit /b 1
)
echo.
echo Capturas listas en build\scene-aerith-review.
pause
