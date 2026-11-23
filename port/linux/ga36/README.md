# ReKHCoM para GA36 Allwinner

Este ZIP manual usa Linux ARMv7 hard-float y el framebuffer de la consola. Apunta a la GA36-MB con Allwinner A33 y al mismo mínimo de CPU en las variantes A23. La inscripción exterior y la revisión impresa en la placa pueden compartirse entre variantes. El paquete para R36S RK3326 usa AArch64 y tiene su propio launcher. [GA36-MB V1.2 BSP](https://github.com/ricardomm7/ga36-bsp) · [goodluckOS](https://github.com/CodeZombie/goodluckOS)

## Instalación

1. Extrae el ZIP en la carpeta `ports` que ya usa el frontend de tu firmware. En EmuELEC suele ser `/storage/roms/ports`. Deja `ReKHCoM GA36.sh` junto a `rekhcomga36/`.
2. Copia tu propio volcado GBA europeo, estadounidense o japonés a `rekhcomga36/roms/`.
3. Abre **ReKHCoM GA36** en **Ports** si tu firmware muestra scripts. Por terminal también puedes ejecutar `sh '/ruta/ports/ReKHCoM GA36.sh'` y añadir `--region eu`, `us` o `jp`.

El juego busca `/dev/fb0` con acceso de lectura y escritura. Las opciones de inicio usan relación 4:3, zoom 2× y filtro de punto para el área extendida. Usa **Start + Select** para abrir las opciones y selecciona **Salir**. Si tu firmware necesita un mapeo SDL específico, coloca `gamecontrollerdb.txt` en `rekhcomga36/` o usa `SDL_GAMECONTROLLERCONFIG_FILE`. Las preferencias, partidas y `ReKHCoM.log` quedan dentro de esa carpeta.

El paquete usa su propio loader ARM y SDL3. Las notas de glibc declaran ABI de kernel Linux 3.2.0. El firmware de fábrica documentado usa sunxi 3.4.39. Arranque en emulación y auditoría de ABI verifican la compilación, mientras que acceso a framebuffer, orientación, mandos, audio y rendimiento quedan sujetos a la consola y su firmware. La instalación sólo introduce archivos del juego en `ports`.

## Compilación

Prepara `linux/armhf` con el sysroot, SDL3 y FreeType de Linux del proyecto y pon tus volcados verificados en el checkout para generar los ejecutables regionales. Ejecuta desde la raíz del repositorio.

```sh
python port/tools/build_linux.py --arch armhf --versions eu us jp --jobs 1
python port/linux/ga36/package.py build/ReKHCoM-ga36-armhf-alpha.zip
```

Para perfilar el renderizado en varios núcleos, añade `--armhf-ppu-workers` a
`build_linux.py`. En ese binario, `KHCOM_PPU_THREADS=4 KHCOM_PPU_INNER=1`
activa el reparto de filas en una A33. `KHCOM_PPU_VERIFY=1` las vuelve a
dibujar en serie y registra diferencias, así que se usa en una pasada
separada del benchmark. La compilación del ZIP normal mantiene su perfil
serial mientras falta la validación física.

El instalador y el juego aceptan `SDL_AUDIO_DRIVER` y `KHCOM_FBDEV` para diagnósticos. `KHCOM_FBDEV` vacío habilita la prueba sin framebuffer junto a un driver SDL apropiado. Conserva el log de una sesión, el firmware, el nombre del dispositivo de entrada y el resultado de la prueba física al comparar rendimiento. [Opti36S](https://github.com/MatyMeatBoy/Opti36S)
