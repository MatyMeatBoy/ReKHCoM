# Comandos del workflow de arte amplio

PowerShell desde `C:\Users\MP\Documents\00 Claude\KHCoMPC\khcom-codex`.
Ejecutar **una orden de compilación/juego a la vez** y esperar su final.
Ejemplo real 121: cambiar id/directorio al revisar otra pieza. No sobreescribir
evidencia aprobada. Los comandos producen datos; hay que abrir las imágenes.

## Identificar y compilar

```powershell
git status --short
rg -n 'gEventBg121' src/evt/event_index_data.c
python port/tools/inventory_preliminar_v1.py --region eu
```

Inventario: `build/preliminar-v1/inventory/eu/scenes.json`, `contact.png` y
`gEventBg121.png`. El JSON declara eventos, dimensiones, hashes y bounds.
Repetir `--region us`/`jp` si falta el inventario regional.
`scene_tile_data.py` es biblioteca de decodificación (`bg_symbol`, `load_bg`),
no tiene argumentos de exportación. Para estudiar capas usar sus grids
`bg3/bg2/bg1`, tiles, flip bits y paletas; conservar transparencia de índice0.

Tras editar solo la receta de la pieza:

```powershell
python port/tools/preliminar_v1_recipes.py --generate build/preliminar-v1/inventory/eu/scenes.json build/preliminar-v1/inventory/us/scenes.json build/preliminar-v1/inventory/jp/scenes.json
..\toolchain\bin\ninja.exe -f build/agrabah109-eu/build.ninja -j 1
```

`READY` controla activación. Una habilitación local para estudiar una receta
nueva no autoriza entregarla como cerrada ni incluirla en el commit de otra.

## Primera muestra; luego matriz

```powershell
python port/tools/test_preliminar_v1.py --build build/agrabah109-eu --out build/preliminar-v1/revision-121-v2-quick --region eu --only escena-121 --quick
```

Abrir `revision-121-v2-quick/escena-121/3440x1440-300-despues.png` y `-antes.png`.
Corregir la geometría/detalle. Después ejecutar, en serie:

```powershell
python port/tools/test_preliminar_v1.py --build build/agrabah109-eu --out build/preliminar-v1/revision-121-v2-eu --region eu --only escena-121
python port/tools/test_preliminar_v1.py --build build/agrabah109-eu --out build/preliminar-v1/revision-121-v2-original --region eu --only escena-121 --original
..\toolchain\bin\ninja.exe -f build/agrabah109-us/build.ninja -j 1
python port/tools/test_preliminar_v1.py --build build/agrabah109-us --out build/preliminar-v1/revision-121-v2-us --region us --only escena-121 --quick
..\toolchain\bin\ninja.exe -f build/agrabah109-jp/build.ninja -j 1
python port/tools/test_preliminar_v1.py --build build/agrabah109-jp --out build/preliminar-v1/revision-121-v2-jp --region jp --only escena-121 --quick
```

Esto verifica matriz EU + Original EU y **muestras rápidas** US/JP. Para matriz
regional quitar `--quick`; para Original regional ejecutar también `--original`.
No atribuir cobertura que no se ejecutó.

Fondos compartidos: listar todos los fixtures existentes en `fixtures()` del
harness. Ejemplo: `--only escena-118,evento-119-escena-118`. Añadir un inicio
que falte antes de declarar cobertura completa del fondo.

El harness guarda 100/300/1000 y reproduce cada instantánea con el mismo binario.
El «antes» apaga solo la pieza con `KHCOM_PRELIMINAR_SKIP`; las anteriores siguen
activas. Controles para la pieza 121:

```powershell
python port/tools/test_preliminar_v1.py --build build/agrabah109-eu --out build/preliminar-v1/revision-121-v2-controles --region eu --subject 121 --only escena-004,escena-057,escena-061,escena-074,escena-107,escena-109 --quick
```

Los controles exigen cero cambios en toda la imagen. Si cambia el renderer
común, ampliar controles/fases afectados. Guard Armor tiene fixtures/herramientas
propios: consultar `audit_battle_scenery.py` y `BATTLE_SCENERY_TECHNIQUE.md`.

## Trazabilidad y entrega

```powershell
Get-FileHash -Algorithm SHA256 build/agrabah109-eu/khcom.exe
git rev-parse HEAD
```

Guardar crops de uniones junto al panorama indicando `(x0,y0,x1,y1)` y si las
coordenadas son del panorama x3 o del lienzo fuente. Usar vecino más próximo;
las imágenes `-limpio.png` del harness están ampliadas 3:1 sin filtro.

Después de las puertas visual/técnica, conservar informes finales EU, Original
y regionales en la evidencia de la pieza. El informe principal consumido por
el paquete es el de la matriz amplia EU; no sustituirlo por el rápido/Original.

```powershell
python port/tools/package_preliminar_v1.py --build build/agrabah109-eu --evidence build/preliminar-v1/ready --out ../ReKHCoM-preliminarV1
```

El empaquetador comprueba `passed` y al menos 12 checks por fixture; no comprueba
calidad visual, cobertura regional ni identidad de binario. Esos requisitos
deben estar verificados en la ficha antes de ejecutarlo. Copia privada, no
publicación ni integración; Codex hace commits y Claude integra.
