# Pendientes vigentes hacia la v2.0 (lista limpia, 26/10/2026)

Solo lo abierto. Lo terminado y los traspasos anteriores: **archivo/PENDIENTES_ARCHIVO_20261026.md** (y los
archivos anteriores). Método de depuración: DEBUG_WORKFLOW.md. Cola de arte: `python port/tools/art_queue.py --quiet
list` (Atlas). Reglas del usuario: REGLAS_DEL_USUARIO.md. Última prerelease: **v2.0-pre19** (26/10, Windows, Linux,
Steam Deck; zip limpio en KHCoMPC).

## Reglas que no se olvidan

- Rama y PR por entrega; lo visual pasa por Atlas y solo entra lo aprobado (LOGICA_APROBADOS.md).
- Publicar (releases, comentarios) con OK del usuario; la estable solo con su OK.
- Nunca borrar releases ni carpetas del usuario; trabajar en KHCoMPC (C:) y D:/01Claude/ReKHCoM-build.
- Un timeout o fallo de batería es un bug: reproducir (antes, mirar kh_com_host.log: ¿le falta la imagen?).
- Responder al usuario en español y breve.

## 1. Código (Claude)

- [x] **#202 Modo CRT (26/10):** vista CRT 320x240 con salida 240p nativa, superresolución x6/x8 y 480p 2x, en juego y launcher; test enviado a itsNateDev (rama test-builds), esperando su prueba.
- [ ] **Baterías** (khcom-claude-int, `run_batteries.py --status`): relanzadas el 26/10 con marcas frescas; su parte
  Android trae Agrabah 123/124 (app smoke completa). Revisar resultados y arreglar lo que salga.
- [ ] **Antes de la 2.0** (sección 3): correr la lista de pruebas sobre el commit final.

## 2. Arte (Atlas — Codex)

- Cola: arena de Agrabah (frente), tienda moguri, cambiar naipes, añadir naipes (esquina), botón Opciones del menú,
  banner de Steam, escena 087, y lo que Codex tiene en curso (057, 061, 074, 097, diario, Maléfica, remates).
- Al entregar el botón Opciones: el código lo agrega al menú y quita Start+Select para Opciones (solo menú y título).
- Título HD hasta cargar la partida y Opciones en HD con el HUD HD: diseño en Atlas, luego código.

## 3. v2.0 estable (solo con OK del usuario)

- `release_check.py v2.0` antes y `--post` después; `check_sources.py`.
- Builds desde el commit final: Windows, Linux, Steam Deck, Android, PS4, Vita, PS5 (VM), web (sin Switch).
- Pruebas: `test_update.py` (baja las estables de GitHub), `test_hd_audio.py`, `test_hd_audio_flow.py`,
  `test_frameskip.py`, `test_hd_scenes.py`, `match_hd_scenes.py --check`, `test_launcher.py`, smoke de cada exe.
- Notas cortas que anuncien el mod HD; `announce_release.py v2.0` para X, Reddit y Patreon (publicar con OK).

## 4. Esperando a otros (no se trabaja hasta que llegue)

- #163 duartepedro: revisión de la galería de escenas y del test3 (videos HD desde el segundo justo).
- Material del usuario para los modos PS2 con REA (ISO/ELF de PS2 y carpeta 1.5 HD).
- Reportes: cara HD de Sora con el pelo cortado al recibir daño; cuadro negro en Agrabah con HUD HD (falta captura);
  Vita: undub por cabecera y velocidad.

## 5. Después de la v2.0

- Modos de Re:CoM basados en PS2 con REA: mejorar o arreglar los modos que ya hay (no modos nuevos).
- Linux/Steam Deck 64 bits (el arreglo de GPU ya funciona); consolas a más potencia solo con ajustes documentados.
- iOS (IPA por fuera de la App Store; hace falta una Mac para compilar).
- Switch (SWITCH_MOTOR.md); Vita/PS4 a la GPU; online (~70 %); rutas de Sora y Reverse/Rebirth (#39).
