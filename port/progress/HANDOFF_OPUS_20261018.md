# Relevo a Opus — 18/10/2026 (noche)

Sustituye a HANDOFF_CLAUDE_20261018.md. El usuario habla español y exige seguir
sus instrucciones al pie de la letra. Cola única: PENDIENTES_VIGENTES.md.

## Dónde está todo

- Tu checkout: `khcom-claude-int`, rama `claude/next`, igual a `origin/main`.
  Publicar con `git push origin claude/next:main` (rebase antes si Codex subió).
- Codex trabaja en `khcom-codex` (main). No tocar su árbol; si lleva >10 min sin
  commits ni archivos tocados, tomar sus puntos anotando hora y evidencia.
- `khcom` (rama feat/native-movies) tiene WIP viejo del 14/10 sin commitear
  (HD HUD, archivos modificados). No es trabajo actual; no commitear a ciegas.
- Toolchain: `../toolchain/llvm-mingw-*/bin` y `../toolchain/bin` en PATH;
  `python port/configure_pc.py --version eu|us|jp --out build/pc-<v>` y
  `ninja -f build/pc-<v>/build.ninja`. ROMs en `roms/` (ignoradas por git).
- Harness: siempre `SDL_VIDEO_DRIVER=dummy SDL_AUDIO_DRIVER=dummy`.

## Publicado

- Nightly 5 `v1.9.3-nightly.20261018.5` (prerelease): NIGHTLY5_PUBLICACION.md.
- **v1.9.4 estable**: ver sección «Estado de v1.9.4» al final (se completa al publicar).
- Receta de publicación: bump `REKHCOM_VERSION` en `port/decompui/decompui.cpp`,
  CHANGELOG, etiqueta de `port/web/index.html`; `build_all.py --platform all --package
  --release-version vX --web-sdl-build D:/01Claude/ReKHCoM-build/web-sdl --web-work D:/01Claude/ReKHCoM-build/web`
  (PS5 no: requiere host Linux); zip de `build/web` como `-web-alpha.zip` con carpeta
  raíz; SHA256SUMS; `gh release create` (`--prerelease --latest=false` para nightly);
  `python port/tools/publish_web.py --sdl-source ../toolchain/SDL3-3.4.18 --publish`
  (exige árbol limpio). Android: icono ya incluido desde v1.9.1.

## Cómo trabajar con el usuario

- Todo lo visual: copia jugable aparte + `.bat` (PRUEBAS_VISUALES.md), esperar su
  aprobación, documentar en LOGICA_APROBADOS.md (lógica del borde de Riku aprobada).
- Hardware real NO es pendiente: llega como reporte.
- Publicado = sale de la cola. Responder issues de GitHub cuando corresponda
  (inglés, con enlace a la release; imágenes en `docs/issues` con `git add -f`).
- No borrar carpetas ReKHCoM-vX sin que lo pida (18/10 autorizó borrar las
  antiguas dejando 3; respaldo en `backups/saves-releases-borradas-20261018`).

## Qué sigue

Ver PENDIENTES_VIGENTES.md (pasada al limpio). Lo mío: 44 (worktrees/builds viejos,
preguntar antes de borrar: el disco está al 95%) y 39 al final (rutas completas con el
.vbm como guía por tramos). Lo de Codex: 057 playa con mejoras, escenas de jefes,
RetroAchievements, Web en navegador real, audio PSG.
