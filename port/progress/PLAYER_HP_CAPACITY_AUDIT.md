# Auditoría HP jugador (fuente, sin ejecución)

GBA: `InitPlayerProgression`80HP para Sora y Riku; los setups de nueva partida
no lo sustituyen. `LevelUpMaxHp`+15, clamp560 (32 elecciones desde80).
155 es80+5*15, no un máximo final. Debug/link/actores especiales quedan aparte.

El ejecutable privado Re:CoM SHA256
`acf42e96a168c73301f5c2cc2915d07e6c420f56a59c6286655c970bfee36bbf`
contiene float470 enVA1405a6900 y560 enVA1405a6904 (PE leído, no ejecutado).
`hud_ranges.c` FUN_140207... inicializador y FUN_14020c530/update usan esa
proporción para capacidad, verde y rastro. El LAD privado SHA256
`121dc7c15b3494bba21a9b21d2f92d6ffac60e40a84200b00739be9ee7ff173a`
tiene grupos7/8/9: arco0..270, extensión271..470; el cuerpo mide200unidades.

Fórmula actual coincide: floor(clampHP/560*470). A155/155, capacidad y verde
soncuadro130, rastro130 sin daño: está lleno de SU capacidad, no todo el arco.
80→67,280→235,320→268,321→269,322→270,323→271,560→470.
Escala240/640=0.375 común a cara y medidor; cropactual298x184 estable incluye
geometría del LAD según testexistente. No se demuestra un fallo de fórmula/crop
por ver155 corto. Falta comparaciónvisual de referencia ejecutada.

Fallback de imagen usa displayHp/maxHp y todo el track; representa capacidad
de otra manera. No corregirlo simplemente normalizando el medidor LAD ni
cambiando reglasGBA. La equivalencia visual entre ambas rutas sigue pendiente.

GBA displayHp cambia±3porcuadro, gaugeSize por tramos40 y cambia de capa280.
LAD usa HPactual instantáneo, rastro1500ms y cierre gap/10+1, healinginmediato;
facehurt3000ms y low≤floor(max*25/100), conforme lógica portremaster.
Son presentaciones distintas: no alterar HP/daño/curación/juego.

`test_player_hp_capacity.py` PASS: rewardGBA real, controlesPortHdGauge/GaugeParts,
Sora/Riku máximos80..560,155→100 (track130/trail130/green83), trail89pasos sin
cierre/90º149,curación,saltoscuadro,low38/155vs39/155,0HPyreadiness/off.
Sin juego, GPU ni buildpesado. Se detuvo investigación adicional para corregir
P1 de349 y entrega2.1. No se implementó ningún cambioHP.
