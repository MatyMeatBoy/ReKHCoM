# Versiones aceptadas para2.9 — 06/11

Esta integración conserva los commits originales de618/619/621/622. Actualiza
las notas históricas «pending / sin build» de esas propuestas: la valoración
posterior exacta está registrada aquí. No incorpora propuestas aún pendientes.

| PR | Atlas / revisión | Decisión | Comportamiento |
|---|---|---|---|
|618|2f580f52f9ae47d29ea0670711ec5062 /2|approved|árbol095: mismo donante hasta383|
|619|d724dc86778f4d10877ffce94cb07952 /1|honors|bordes081: ciclo32 y celdas vacías BG3|
|621|b3d60c58be8f4f209a4f67243ec8e3c6 /1|honors|grabado120: tinta existente del banco cargado|
|622|9a47fba18d7749a2a54d4b9ccc8da65b /3|approved|flanco Darkside06 exacto211píxeles, activo por defecto|

En622 sólo se modifica el interruptor de activación tras la aceptación; sus
datos indexados y selecciones quedan intactos. `KHCOM_DARKSIDE06_FLANK=0`
permite el control anterior. Original conserva la exclusión del caller.

Seguimientos abiertos: la nube nueva de095 pide retirar un trozo de muro de
pasto; Darkside exige verificar todos los frames del combate. La aceptación
de una pose no certifica la animación. A601 de120 y las hijas pendientes de
Oogie/097 conservan su revisión independiente.

Validación conjunta: fixtures C reales081/120, C095 y tres pruebas de datos
Darkside;1097 predicados C interpretados de activación/bounds/caller. Revisión
de fuentes y resolución de conflictos limitada a conservar ambas secciones
de LOGICA_APROBADOS. QA visual del ejecutable combinado permanece pendiente.
No se modifican ROM, partidas, imports ni valoraciones humanas.
