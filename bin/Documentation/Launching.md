# Launching {#launching}

If you [built a custom branded version of the engine](@ref build-engine), you can run that as is.

Using a generic [FTE](https://www.fteqw.org/) binary however, you need to specify the game directory.
You run `./fteqw` with a parameter specifying the game directory:

```
$ ./fteqw +game base
```

You can dispatch [console](@ref console) commands to be executed once the client is running by appending them to the command-line as launch parameters, prefixed with the `+` character, as seen above.

```
./TestGame_x64 +set g_gametype some_mode +devmap some_level
```

## Mod/Game Setup

A game describes itself (title, start map, singleplayer/multiplayer, ...) in its [FTE](https://www.fteqw.org/)
manifest, `<game>.fmf` next to the game directory, with one `-set gameinfo_<key> <value>` line per setting.
`base.fmf` looks like this:

```
FTEMANIFEST 1
GAME quake
GAMEDIR base

-set gameinfo_game "Nuclide II: Mayhem"
-set gameinfo_title "Mayhem"
-set gameinfo_type "singleplayer_only"
-set gameinfo_startmap "starting_demo"
-set gameinfo_trainingmap "dev"
-set gameinfo_menumap "background01"
```

`gameinfo_type` takes `singleplayer` (or `singleplayer_only`), `multiplayer` (or `multiplayer_only`)
or `both`, which is also the default.
You can find all the keys in GameLibrary_LibListParse() under `src/platform/gamelibrary.qc`.

A game directory without a manifest, or a manifest without `gameinfo_` lines, gets placeholder values
(both singleplayer and multiplayer, no title). GoldSrc `liblist.gam` and Source `GameInfo.txt` files are not read.

Those settings alone can alter quite a bit of the menu options and branding/presentation of your game.
