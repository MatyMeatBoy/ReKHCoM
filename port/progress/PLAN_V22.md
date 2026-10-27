# ReKHCoM 2.2 — traspaso y pendientes

Estado del 27/10/2026. El usuario pidió trasladar aquí lo pendiente después
de publicar 2.1. No seguir ampliando la revisión de 2.1 ni sustituir sus
archivos publicados. Este documento organiza el trabajo; la autoridad de
reservas y responsables sigue siendo `coordination/art-queue`.

## Qué está entregado

- [2.1 estable](https://github.com/MatyMeatBoy/ReKHCoM/releases/tag/v2.1),
  tag `8c613ae5d`: Windows x86/x64, Linux desktop/Deck x86/x64, Android beta
  ARM32/ARM64 y Anbernic armhf beta. Motores compilados en `cd144335e`;
  launcher/empaquetado Linux en `de434db7c`.
- ZIP personal en la raíz KHCoMPC: `ReKHCoM-v2.1-windows-x86-x64.zip`, SHA256
  `1e1d1039b42cf3f12243afe4bd114d5d5d913b117675dd95fbbab53b78baa888`.
  La instalación Quiver principal permanece protegida: no ejecutar ni escribir allí.
- **#356 está integrado en main, pero NO está en 2.1 ni en ese ZIP.**
  Los fondos mínimos GBA/HD sin poster se incluirán en una build de 2.2.
  [Implementación y capturas](GALLERY_EMPTY_BACKDROP.md).
- #354 documentó limpieza y entrega; #355 añadió mediciones y lecciones de
  OptiPort. No confundir esos cambios posteriores con una nueva build publicada.

## OptiPort: porcentaje y límites

**Base conservadora: 10 de 14 bloques originales cerrados/integrados,
aproximadamente 71%; cuatro siguen parciales.** Es un porcentaje de trabajo
de aquella primera batería, no de estabilidad, bugs eliminados ni del alcance
total ampliado. No sumar porcentajes de tareas heterogéneas o declarar 100%
por haber publicado 2.1. El método público y la copia instalada están en
`17f0ffa`; la migración, controles y pulido se registran aparte.

Los diez bloques integrados son logs, portabilidad/alineación, pools/paletas,
imágenes/presupuesto, readiness HUD, música, vídeo, streaming/backpressure,
lifecycle y protección del progreso. Hardware no probado sigue sin verificar.

| Bloque parcial | Evidencia existente | Cierre pendiente para 2.2 |
| --- | --- | --- |
| Profiling | Doce corridas del paquete Windows 32/64; título y combate Monstruo, HD sí/no, percentiles y RSS/private. [Datos](data/v21-initial-profile.json). | Completar sólo recorridos que cambien decisiones: puerta/carga y previews; comparación con igual calidad visual y condiciones, antes de afirmar mejoras. |
| Asíncrono y precarga | Caché/worker de previews #342 y preparación HUD #346; cancelación y resultados obsoletos cubiertos por pruebas de consumidores. | Medir entrada fría/caliente y cambios rápidos GBA/HD; comprobar presupuesto y ausencia de esperas al cerrar. |
| Frame pacing | Primeras ventanas comunes de cuadros 300..600. La variación de 32 bits no identifica por sí sola un bug. | Aislar tirones repetibles y etapa costosa antes de modificar carga o render; no alterar velocidad de la lógica GBA. |
| Entrega y resistencia | Release, manifiestos/hashes y regresiones dirigidas entregados. | Soak acotado de los recursos modificados, gameplay de los paquetes combinados finales Linux/Android y confirmación de dispositivos originales cuando esté disponible. La publicación ya está hecha. |

## Prioridades concretas

| Prioridad | Pendiente y estado real | Siguiente acción mínima |
| --- | --- | --- |
| P0 | **#272 Android Monstruo:** único issue abierto al cierre; evento74 USA pasa en pruebas locales previas, Samsung sin confirmar. | Usar respuesta/log nuevo del reportero; distinguir fallo nativo, lifecycle e import. No atribuir causa confirmada a RAM. |
| P0 | **Cierre del usuario al recuperar foco en Monstruo v2.0:** aún sin reproducción. Es distinto de la puerta 64 bits ya corregida. | Reproducción aislada con foco/pausa y log de sesión conservado; los logs anteriores al cierre original se habían perdido. |
| P1 | **Nombre de ataque del jefe, reproducido y sin corregir.** Axel muestra “Muro de fuego” sobre sus cartas. Claim `hudhd-hechizo-jefe` devuelto a pendiente. | Ajustar layout sólo con HUD HD listo y comprobar fallback GBA. Fuente: `StockNameRiku_2` en `card_friend_card.c`, anclaje central y `DrawSprite(120,14)`. |
| P1 | **Retorno de galería sin flash:** #339 implementado; falta cerrar evidencia completa. | Una matriz dirigida título/partida × GBA/HD × normal/skip, cuadros del retorno y estado de historia/guardado; no repetir todo el juego. |
| P1 | **Dígitos premium y modificados:** #341 protege el fallback multicolor; falta comparación GPU completa. | Carta premium y efecto modificador reales, arriba/abajo, import completo/parcial. [Fixture](HUD_CARD_DIGIT_FALLBACK.md). |
| P2 | **Previews rápidas y fondo mínimo:** #342 y #356 implementados; el fondo no demuestra latencia. | Incluir #356 en la próxima build; medir caché/frío/cancelación y calidad en ambos modos. |
| P2 | **Prompts y controles experimentales:** implementados, default Original; falta QA visual y hardware. | Hotplug, bindings efectivos, todos los botones, modos Nintendo/Xbox/PS/GBA, paletas/localización y opciones no disponibles. |
| P2 | **Media EN/JP:** bibliotecas/import y selección implementados. | Contrastar las fuentes propias EN/JP y offsets, disponibilidad y wording; imports legacy siguen con idioma desconocido. No automatizar descargas Steam sin soporte verificado. |
| P2 | **Launcher Linux básico:** #353 evita depender de Wine para importar; no tiene paridad completa con Windows. | Confirmar flujo con usuario Steam Machine, dependencias y mensajes. El crash original Windows/Proton de #336 no tiene causa diagnosticada aunque exista vía nativa. |
| P2 | **Pulido regular restante:** retratos/cajas de diálogo amplias, HUD contra jefes y otros detalles sin cierre. | Mantener los tickets existentes de [pendientes](PENDIENTES_VIGENTES.md); reproducir un defecto por PR, conservar arte y revisión exacta. |
| P3 | **Linux ARM64:** builder fuente experimental; no es el binario de Deck ni Anbernic armhf. Consolas sin nuevas builds 2.1. | Construir/verificar cada plataforma que se vaya a ofrecer; publicar sólo artefactos reales y mantener beta donde corresponda. |
| P3 | **Dummies y worktrees:** primera limpieza recuperó 15.645.146 bytes de cuatro duplicados exactos. | Archivar o mover a `khcom-worktrees` sólo checkouts inactivos, después de preservar WIP, ignorados útiles, ROMs, partidas y evidencia. No borrar por nombre `dummy`/`stub`. |

Repro del nombre de Axel: ejecutable/guest publicados Windows64 EU, HUD/cartas
HD, import aislado con cero stale; `KHCOM_BATTLE=186:12:0`, `KHCOM_MODE=100:noharm`,
`KHCOM_KEYS=60-63:A`. Captura464, nombre85 y `rikuStockNameShown=1`, en
`khcom-crash-diagnostics/build/boss-label-before/axel/host_shot_464.bmp`;
`report.json` conserva el intervalo activo432..560. No hay arreglo de layout
commiteado y no se ha iniciado otra build para este pendiente.

## Cómo continuar sin repetir trabajo

Tomar un claim, reproducir el pendiente específico y elegir un cambio pequeño;
verificar su consumidor y una captura cuando sea visual, abrir PR e integrar
con lock. No repetir la matriz de submenús ya capturada, recompilar todas las
plataformas por documentación ni perseguir ruido de una medición aislada.
Actualizar el porcentaje sólo al cerrar un bloque con evidencia. Mantener
separados implementado, verificado, integrado, publicado y confirmado por el
reportero. Llevar a OptiPort sólo lecciones generales comprobadas.
