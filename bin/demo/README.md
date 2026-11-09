# demo: minimal FTEQW game template

A first-person player in an empty room lit by one `light` entity. The player walks (WASD),
jumps (SPACE) and looks around with the mouse. The game is plain QuakeC on the generic engine
`bin/fteqw.exe`, with no SDK on top.

## Build and play

From `bin\` (cmd or PowerShell; it builds through MSYS2 UCRT64):

```bat
run.bat -map room          :: compile the QuakeC, then play maps/room.bsp
run.bat run -map room      :: play without compiling
```

From an MSYS2 UCRT64 shell at the repo root, or with the VS Code tasks ("Game: ..."):

```sh
./build_win64.sh game      # src/progs.src -> progs.dat + csprogs.dat
./build_win64.sh maps      # maps/*.map -> maps/*.bsp (vmap: bsp, vis, light)
```

Without `-map`, the engine opens its menu; "New Game" loads `room` (`startmap_sp` in `default.cfg`).

## Layout

| Path | What |
|---|---|
| `src/progs.src` | Build entry: `#pragma sourcefile` for each program |
| `src/server/` | Server game (SSQC) -> `progs.dat` |
| `src/server/sv_main.qc` | Engine entry points (`StartFrame`, `ClientConnect`, `ClientKill`...) |
| `src/server/sv_player.qc` | `PutClientInServer` (spawn at `info_player_start`) and `SV_RunClientCommand` (movement + jump sound) |
| `src/server/sv_entities.qc` | Spawn functions for the map's classnames: `worldspawn`, `info_player_start`, `light` |
| `src/server/defs.qh` | Player box, eye height, jump button, sound names |
| `src/client/cl_main.qc` | Client game (CSQC) -> `csprogs.dat`: draws the HUD line |
| `src/fteextensions.qc` | Engine API definitions for both programs (generated, see below) |
| `maps/room.map` | The room: 6 brushes, `info_player_start`, `light` (Quake 3 brush format, for vmap) |
| `textures/demo/` | `floor.tga`, `wall.tga` used by the map |
| `quake.rc`, `default.cfg` | Startup script and default binds; they override LibreQuake's copies in the paks |
| `pak0.pak`, `pak1.pak` | LibreQuake data (console font, palette, sounds). Not in git |
| `../demo.fmf` | Engine manifest (game name, game dir) |

Generated and ignored by git: `progs.dat`, `csprogs.dat`, `*.lno`, `maps/*.bsp`, `maps/*.srf`.

## How it works

- **Movement.** `SV_RunClientCommand` calls `runstandardplayerphysics(self)`, the engine's
  player physics: acceleration, friction, gravity, stairs, and jumping while `+jump` is held
  on the ground. The client predicts with the same code, so movement stays smooth.
  To change movement, edit the `input_*` globals before that call, or replace the call.
- **Rendering.** `cl_main.qc` uses FTE's "simple CSQC": since it does not define
  `CSQC_UpdateView`, the engine draws the 3D view and calls `CSQC_DrawHud` for the 2D overlay.
  Define `CSQC_UpdateView` to take over the camera and HUD.
- **Light.** vmap's light stage bakes the `light` entity into lightmaps. At runtime its spawn
  function removes it.
- **Startup.** `quake.rc` runs `default.cfg` and `autoexec.cfg` (optional, per user). It must not
  call `stuffcmds`: FTE runs the command-line `+commands` itself after startup, and running them
  from `quake.rc` loses `+map`. `default.cfg` sets `sv_progs progs`, so the server always loads
  this `progs.dat` and never the `qwprogs.dat` from the pak.

## Extending it

- New entity: add a spawn function named after its classname in `sv_entities.qc`
  (or in a new file listed in `src/server/progs.src`).
- New sound or model: precache it in `worldspawn`.
- New map: add `maps/<name>.map` (any vmap-compatible editor), then `build_win64.sh maps`.
- New engine version: regenerate `src/fteextensions.qc` from the engine console with
  `pr_dumpplatform -O fteextensions` (it writes `demo/src/fteextensions.qc`).
