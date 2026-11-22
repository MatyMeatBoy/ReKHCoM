# Atlas: aceptados del06/11, preparación sin build

Se integran las siete versiones exactas seleccionadas por Atlas. No se compila
C, no se ejecuta el juego, no se regeneran imports ni se publican paquetes.
La futura pre2.8.5 sigue pendiente de build y QA combinada autorizados.

| Pieza | PR original | Imagen | Revisión/grado | Seguimiento |
|---|---|---|---|---|
| Escena 095 | #600 | `e78f689cba2e46548b67cff57c10f444` | 2/approved | verificar que el arbol del inicio de la escena no este solo su mitad |
| Escena 115 | #602 | `bb7866a6068d4e478a3e19fc98bc423b` | 3/approved | falta extender esta diagonal a derecha; falta extender esta diagonal a izquierda |
| Escena 081 | #603 | `9224a996d76947d48204e2501ede8e80` | 3/approved | mejorar patron; mejorar patron |
| Escena 130 · Bastión Hueco · Biblioteca de Bella | #604 | `e94de270ac4f4273bd8d5a874bd808ba` | 2/honors | ahora falta extender este a la derecha si |
| Escena057: madera superior y matas completas | #605 | `95ec0b98d0ea4064a9fa44a95f5c1467` | 1/approved | Sin nuevas notas |
| Agrabah 107 / 108 | #608 | `5b37fecc82d24afd914675cbd383204b` | 1/honors | Sin nuevas notas |
| Diario Pepito: lista interior y detalle de carta | #609 | `0f89b2fa9a1443df99343bf8047f9a1a` | 2/approved | este creo que debia llenar has la izquierda si no me equivoco |

Las aprobaciones con comentarios se integran según la política accepted de Atlas;
sus comentarios siguen pendientes, no se consideran respondidos por este merge.
Las propuestas120(PR601) yOogie(PR606) no se incluyen porque su última versión
sigue pendiente. Pepito requiere todavía el seguimiento del margen izquierdo.

Verificación: SHA256 de las siete imágenes servidas y revisión enviada exacta;
heads originales contenidos por merge; fuentes individuales conservadas; sólo
conflictos de append de LOGICA conciliados. Evidencia y estudios anteriores
conservados en pruebas/atlas-siguiente-20261105/. No se atribuyen esos modelos
a un ejecutable combinado. Evidencia actual pruebas/atlas-20261106/.
