# Menus {#menus}

The menu is its own progs, `<game>/menu.dat`, built from `<game>/src/menu/`. It runs separately from the
client and server progs: it talks to the game through console commands (`localcmd`) and cvars.

The SDK does not ship a menu. It ships the pieces to build one, and each game owns its menu:

| Where | What |
|---|---|
| `src/vgui/` | widgets: windows, buttons, labels, lists, tabs, sliders, text boxes, 3D views |
| `src/platform/` | services without UI: game/mod library, map library, saved games, servers, music, localisation |
| `src/menukit/` | **MenuKit**: the menu runtime (engine entry points, desktop, main-menu buttons, loading screen) |

A game picks one of two routes:

- **MenuKit + VGUI**: describe the menu in a few calls and build submenus out of VGUI windows. Start from
  `src/menukit/example/`.
- **A menu of its own**: write the engine entry points yourself. `base/src/menu/` is such a menu, a recreation
  of the Half-Life WON menu with its own widgets, and is free to change per game.

## Building a menu with MenuKit

`<game>/src/menu/progs.src`:

```
#pragma target fte_5768
#pragma progs_dat  "../../menu.dat"
#define MENU
#define GAME_DIR "mygame"

#includelist
../../../src/menukit/includes.src
menu.qc
#endlist
```

`<game>/src/menu/menu.qc`:

```
void
Game_MenuInit(void)
{
	Menu_ShowGameTitle();

	Menu_AddButton("Resume Game", "gfx/icon16/control_play", Menu_Resume, MENU_INGAME);
	Menu_AddButton("New Game", "gfx/icon16/new", MyGame_NewGame, MENU_ALWAYS);
	Menu_AddSpacer();
	Menu_AddButton("Disconnect", "gfx/icon16/disconnect", Menu_Disconnect, MENU_INGAME);
	Menu_AddButton("Quit", "gfx/icon16/stop", Menu_ShowQuitDialog, MENU_ALWAYS);
}
```

Buttons stack in the order they are added. `MENU_OFFLINE` buttons show in the main menu, `MENU_INGAME`
ones in the pause menu, `MENU_ALWAYS` in both; the column closes up around the hidden ones.

### Submenus

A submenu is a `vguiWindow`. Create it once, fill it with widgets, and show it with `Menu_ShowCentered()`:

```
void
MyGame_NewGame(void)
{
	static vguiWindow winNewGame;

	if (!winNewGame) {
		winNewGame = spawn(vguiWindow);
		winNewGame.SetTitle("New Game");
		winNewGame.SetSize([256, 160]);
		/* ... add vguiButton / vguiRadio / vguiLabel children with winNewGame.Add() ... */
	}

	Menu_ShowCentered(winNewGame);
}
```

### Hooks

| Hook | Required | Default |
|---|---|---|
| `Game_MenuInit()` | yes | a bare menu (Console, Resume, Quit) and an error in the console |
| `Game_MenuDraw(screenSize)` | no | the background map or image |
| `Game_MenuLoading(screenSize, isOpaque)` | no | the VGUI loading panel |
| `Game_MenuConsoleCommand(name, argc)` | no | not handled; register the command with `registercommand()` in `Game_MenuInit()` |

### Stock actions

`Menu_Resume`, `Menu_Disconnect`, `Menu_ShowQuitDialog` and `Menu_ShowConsole` can be passed straight to
`Menu_AddButton()`. The `console` and `menu_quit` console commands open the latter two.

The full API is documented in `src/menukit/menukit.qh`.
