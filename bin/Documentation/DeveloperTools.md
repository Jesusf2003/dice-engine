# Developer Tools {#devtools}

The SDK ships a set of tools for building and debugging a game: editor windows, console commands
that inspect or poke at the running game, debug drawing, and the shell scripts of the asset pipeline.
The in-game ones live in their own optional module, `src/tools/`.

## Building them in

A game opts in by adding the two lists at the **end** of its progs sources, after the SDK code
(the tools use globals defined there, such as the VGUI desktop):

```
/* client: <game>/src/client/progs.src */
../../../src/client/include.src
../../../src/shared/include.src
../../../src/tools/client.src

/* server: <game>/src/server/progs.src */
../../../src/server/include.src
../../../src/shared/include.src
../../../src/tools/server.src
```

Leave them out (for example in a release build) and the game still compiles and plays; the commands
below simply don't exist, so a player can't use the cheats among them either.

The core only reaches the module through four `__weak` hooks, which the module overrides:

| Hook | File | Called |
|---|---|---|
| `Tools_Init()` | `src/client/tools.qc` | from `CSQC_Init`: registers the tool commands and the waypoint text menus |
| `Tools_ConsoleCommand(string)` | `src/client/tools.qc` | for a client console command the core and the game didn't handle |
| `Tools_ServerCommand()` | `src/server/tools.qc` | for an unknown server command (`sv <command>`) |
| `Tools_ClientCommand(ncClient, string, int)` | `src/server/tools.qc` | for an unknown command a client sent (`cmd <command>`) |

| Folder | Contents |
|---|---|
| `src/tools/client/` | `EditorSun.qc`, `Editor.qc`, `vgui_modelspawner.qc`, `way.qc`, `commands.qc` |
| `src/tools/server/` | `debugdraw.qc`, `commands.qc` |

Most of the commands only make sense on a local game (singleplayer or listen server): several client
commands just forward to `sv ...`, which runs on the local server.

## Editor windows

| Command | What it does |
|---|---|
| `showSunEditor` | Sun / time of day editor: sliders for the color, pitch, yaw, roll and shadow distance of a preview sun (an `ncCascadeLight` spawned on the client). Use it to find values for the map's sun entity. |
| `showModelSpawner` | Model viewer: a 3D view of any model with its sequences, renderer options and camera. *Spawn Prop* asks the server for `sv propSpawn <model>`; *Set ViewModel* puts the model on your first-person view model; *Spawn Doll* drops it as a client-side ragdoll at the camera (like `ragdollCreate`). |
| `showEditor` | In-game editor frame with select / move / rotate buttons. Work in progress: the buttons don't do anything yet. |
| `way_menu` | Waypoint editor as a text menu (navigate it with the `slot` keys). Loads `data/<map>.way` first. See @ref bot_waypoints. |

## Inspecting the game

| Command | Side | What it prints |
|---|---|---|
| `getpos` | client | Camera position and angles, as a `setpos ...;setang ...` line you can paste back |
| `listEntitiesByClass <classname>` | server | Number, origin and bounds of every entity of that class |
| `listEntitiesByTarget <name>` | server | Entities whose `target` is `<name>` |
| `listTargets` | server | Every entity with a `targetname` |
| `listEntityData [number]` | server | All the fields of an entity; without a number, the one you're looking at |
| `listEntitySpawnData [number]` | server | The key/values the entity spawned with; without a number, the one you're looking at |
| `listEntityDef`, `listServerEntityDef` | server | The entityDefs the server loaded |
| `listClientEntityDef` | client | The entityDefs the client loaded |
| `listServerSoundDef` / `listClientSoundDef` | server / client | Loaded soundDefs |
| `listMapTweaks` | server | Active map tweaks |
| `listModels` | client | Precached models |
| `listSounds` | client | Precached sounds and their total length |
| `listParticles` | client | Precached particle effects |
| `listTitles` | client | Loaded titles (on-screen messages) |
| `listFramegroups <model>` | client | The model's frame groups, as constants ready to paste into code |
| `listInventory` | server | Your inventory and active weapon |
| `listAmmo` | server | Ammo types |
| `listUserInfo` | client | User info keys |
| `sv listGlobals` | server | Session globals: the shared state `env_global`, `multisource`, `trigger_auto` and `logic_auto` read and write |
| `traceMaterial` | server | What the crosshair hits: trace results, surface and material; opens it with `r_showshader` |

## Changing the game

| Command | Side | What it does |
|---|---|---|
| `setpos <x> <y> <z>` | server | Moves you there. Needs `sv_cheats 1`. |
| `setang <pitch yaw roll>` | client | Sets the view angles |
| `teleport <targetname>` | server | Moves you to the entity with that name (repeat to cycle) |
| `teleportToClass <classname>` | server | Moves you to the next entity of that class |
| `spawnDef <name>` | server | Spawns an entityDef where you're looking |
| `sv propSpawn <model>` | server | Spawns that model as a physics prop (the game's `prop_physics` if it has one) |
| `trigger <targetname>` | server | Toggles every entity with that name |
| `input <entity number> <input> [data]` | server | Sends an input to an entity |
| `killClass <classname>` | server | Removes every entity of that class |
| `killMovables` | server | Removes every physics object |
| `respawnEntities` | server | Respawns the map's entities (and resets `trigger_auto`) |
| `sv renetworkEntities` | server | Sends every entity to the clients again |
| `sv setLightStyle <style> <pattern>` | server | Changes a lightstyle (`a` = dark ... `z` = bright) |
| `sv setGlobal <name> <value>` | server | Sets a session global |
| `giveInventoryItem <item>` | server | Gives you an item or weapon (entityDef name) |
| `removeInventoryItem <item>` | server | Takes it away |
| `removeAllInventoryItems` | server | Empties your inventory |
| `ragdollCreate <model>` | client | Spawns a ragdoll of that model |
| `clearDecals` | client | Removes the temporary decals |
| `reloadClientLocale` | client | Reloads the translation files |

`setpos` is the only one of these that checks `sv_cheats`. Ship a release without the module if that matters.

## Lighting, models and other tests

| Command | What it does |
|---|---|
| `testLight [cubemap]` | A white dynamic light at the camera; with a cubemap texture, a projected one in the view direction |
| `testPointLight` | A dynamic light where you're looking |
| `dev_sunpos` | Points the shadow direction (`r_shadows_throwdirection`) along your view and prints the matching `env_sun` and `light_environment` angles |
| `dev_measure` | Run twice: the first sets a marker, the second shows the distance to it |
| `dev_modeltest <model>` | Places a model at the camera |
| `buildcubemaps` | Builds the map's cubemaps (`env_cubemap`) |
| `titles_test <name>` | Shows a title (on-screen message) |
| `view_geomtest <group> <value>` | Prints the `geomset` line for the view model |
| `player_geomtest <group> <value>` | Sets a geomset on your player model |

## Navigation

`sv way <subcommand>` edits the navmesh directly (it's what `way_menu` runs). New nodes go at your
position; the rest act on the node closest to you. The *two-step* ones are run twice: at the first
node, then at the second.

| Subcommand | What it does |
|---|---|
| `add` | Adds a node, linked to every node in range (`nav_linksize`) with nothing in between |
| `addchain` | Adds a node, linked both ways to the previous one |
| `addltn` / `addntl` | Adds a node, linked one way: previous → new / new → previous |
| `addnear` | Adds a node, linked both ways to the nearest one |
| `addsingle` | Adds a node with no links |
| `addspawns` | Adds a node at every spawn point |
| `delete` | Deletes the closest node |
| `connect1` / `connect2` | Two-step: links the two nodes one way (first → second) / both ways |
| `unlink1` / `unlink2` | Two-step: removes that link one way / both ways |
| `flag <flags>` | Two-step: links first → second and sets the link flags (jump, crouch, walk, aim...) |
| `autolink` | Links the closest node the same way `add` does |
| `radius <size>` | Sets the closest node's radius |
| `move <x> <y> <z>` | Moves the closest node by that much |
| `movetopos` | Moves the closest node to you |
| `offset <x y z>` | Moves every node |
| `flipx`, `flipy`, `flipz` | Mirrors every node on that axis |
| `purge`, `purgelinks` | Deletes every node / every link |
| `save <file>`, `load <file>`, `merge <file>` | Saves / loads / adds a `.way` file (in `data/`) |

Shortcuts on the client:

| Command | Runs |
|---|---|
| `nodeAdd` | `sv way add` |
| `nodeDel` | `sv way delete` |
| `nodeLink` / `nodeUnlink` | `sv way connect2` / `sv way unlink2` (two-step, both ways) |
| `nodeFlags <flags>` | `sv way flag <flags>` (two-step) |
| `nodeRadius <size>` | `sv way radius <size>` |
| `nodeOffset <x> <y> <z>` | `sv way move <x> <y> <z>` |

## Debug drawing and logging

| Cvar | What it does |
|---|---|
| `r_renderEntityInfo 1` | Draws each entity's `DebugDraw()`: its bounding box, plus extras some entities add (lights, path tracks, train paths, scripted sequences, sounds) |
| `nav_visualize 1` | Draws the navmesh: nodes and links |
| `developer 1` | Loads the map's navmesh for the above if needed, and enables other developer output |
| `net_showArrivals 1` | Logs every networked entity as it reaches the client, with the class it became |
| `g_logLevel <n>` | How much the SDK logs (warnings, info, debug...) |
| `nav_linksize`, `nav_radius` | Link range and default node size for the navmesh editor |

The debug drawing (`src/tools/server/debugdraw.qc`) runs on the server but draws through the local
client, so it only works in singleplayer or on a listen server.

## Pipeline scripts

The shell scripts in `Tools/` are separate from the module; they run on the host, not in the game:

| Script | What it does |
|---|---|
| `make_mapdef.sh <game>` | Writes `<game>/scripts/entities.def` for the level editor, from the `QUAKED` comments of the SDK and the game and from the game's entityDefs |
| `make_map.sh`, `make_maps.sh <game>` | Compile one map / every map of `<game>/mapsrc/` |
| `make_models.sh <game>` | Builds the models of `<game>/modelsrc/` |
| `make_textures.sh <game>` | Builds the textures of `<game>/texturesrc/` |
| `build_wads.sh` | Builds the texture wads |
| `make_trshaders.sh` | Converts a game's `.mat` files to `.shader` files and rewrites `scripts/shaderlist.txt` |
| `make_dist.sh` | Packs the archives from the `.pk3dir` directories |
| `make_launcher.sh <dir> <command> <destination>` | Writes a launcher |
| `get-name.sh`, `get-plugins.sh <game>` | Read a game's `PROJECT` and `PLUGINS` files (used by the build) |
