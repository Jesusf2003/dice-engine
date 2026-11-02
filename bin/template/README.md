# Template

The starting point for a standalone game made with the engine and QuakeC.
It runs on its own: `template.fmf` makes this folder the only game folder, so
nothing from ninjawars (or `default.fmf`) is loaded.

What it does:

- loads `maps/start.map`, a closed room lit by one `light` entity;
- the player is in first person, walks with WASD and jumps with SPACE (the
  engine's movement, `runstandardplayerphysics`);
- messages go to the console and to the HUD:
  - `sprint`/`bprint` from the server show in the console and in the HUD's
    message list;
  - `centerprint` shows in the middle of the screen;
  - the `hud_say <text>` console command (CSQC) does both.

## Running

`run.bat` compiles the QuakeC with `fteqcc64.exe` and starts `fteqw64.exe`:

```
run.bat              compile and run maps/start
run.bat build        compile only
run.bat run [map]    run only, another map
```

If `maps/<map>.bsp` exists it loads that. If not, it loads `maps/<map>.map`
directly: the engine reads uncompiled `.map` files. A `.map` has no
lightmaps, so `default.cfg` turns on realtime lighting
(`r_shadow_realtime_world`) and its light entities light it. Once the map is
compiled to a `.bsp` (qbsp, vis, light), the lightmaps take over.

## Files

| | |
|---|---|
| `template.fmf` | the manifest: the game's name and its only folder |
| `default.cfg` | key bindings and the engine settings the game starts with |
| `template.wad` | the textures (`floor`, `wall`, `ceiling`) |
| `template.fgd` | the entities, for the map editor (radiant's FGD dialect) |
| `maps/start.map` | the room |
| `src/progs.src` | compiles `client.src` (CSQC → `csprogs.dat`) and `server.src` (SSQC → `progs.dat`) |
| `src/shared/sh_defs.qc` | constants both sides use: the player's size, the custom stats |
| `src/server/sv_main.qc` | the server's entry points (`worldspawn`, `PutClientInServer`, `SV_RunClientCommand`...) |
| `src/server/sv_player.qc` | spawning the player, noticing its jumps |
| `src/server/sv_entities.qc` | the map's entities: `info_player_start`, `light` |
| `src/client/cl_main.qc` | the client's entry points: the view, the console command, intercepting prints |
| `src/client/cl_hud.qc` | the HUD: stats, messages, centerprint |
| `src/fteextensions.qc` | the engine's builtins and constants (generated, don't edit) |

## Starting a new game from it

1. Copy the folder and rename it, for example `bin/mygame`.
2. Rename `template.fmf`, `template.wad` and `template.fgd`, and change
   `template` to the new folder's name in the `.fmf`.
3. In `run.bat`, change the `.fmf` name. In `maps/start.map` and the `.fgd`,
   change the wad's name.

## In the map editor

Tools > Options > Detect adds a game profile for this folder, because it has
an FGD in it. Maps open from `template/maps`.

The editor's Run > Launch Engine still starts the engine with `default.fmf`.
To try a map as a standalone game, use `run.bat run <map>`.
