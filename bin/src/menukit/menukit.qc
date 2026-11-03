/*
 * Copyright (c) 2016-2022 Vera Visions LLC.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define MENUKIT_MAXBUTTONS 32

static vguiWidget g_menuDesktop;
static vguiMenuButton g_menuButtons[MENUKIT_MAXBUTTONS];	/* __NULL__ entries are spacers */
static menuVisibility_t g_menuButtonVis[MENUKIT_MAXBUTTONS];
static int g_menuButtonCount;
static var int g_menuLayoutState = -1;	/* Menu_InGame() the buttons were last laid out for */
static vguiLoadingPanel g_menuLoadingPanel;

/* vguiWidget::Add() appends to a linked list, so adding a widget twice would loop it */
.bool m_menukitAdded;

/* game hook defaults -------------------------------------------------------- */

__weak void
Game_MenuInit(void)
{
	ncError("Game_MenuInit() is not implemented by the game, using a bare menu.");
	Menu_AddButton("Console", "gfx/icon16/application_xp_terminal", Menu_ShowConsole, MENU_ALWAYS);
	Menu_AddButton("Resume Game", "gfx/icon16/control_play", Menu_Resume, MENU_INGAME);
	Menu_AddButton("Quit", "gfx/icon16/stop", Menu_ShowQuitDialog, MENU_ALWAYS);
}

__weak void
Game_MenuDraw(vector screenSize)
{
	Background_Draw([0,0], screenSize, false);
}

__weak void
Game_MenuLoading(vector screenSize, float isOpaque)
{
	if (isOpaque || (clientstate() != 2)) {
		Background_Draw([0,0], screenSize, isOpaque);
	}

	if (!g_menuLoadingPanel) {
		g_menuLoadingPanel = spawn(vguiLoadingPanel);
	}

	g_menuLoadingPanel.Reposition();
	g_menuLoadingPanel.Draw();
	g_menuLoadingPanel.UpdateProgress();
}

__weak bool
Game_MenuConsoleCommand(string commandName, int argCount)
{
	return (false);
}

/* building the menu ------------------------------------------------------- */

bool
Menu_InGame(void)
{
	return (clientstate() == 2 && !g_background) ? (true) : (false);
}

vguiWidget
Menu_Desktop(void)
{
	return (g_menuDesktop);
}

void
Menu_Add(vguiWidget toAdd)
{
	if (toAdd.m_menukitAdded == true) {
		return;
	}

	toAdd.m_menukitAdded = true;
	g_menuDesktop.Add(toAdd);
}

void
Menu_ShowCentered(vguiWindow window)
{
	Menu_Add(window);
	window.Show();
	window.SetPos((g_vidsize / 2) - (window.GetSize() / 2));
}

void
Menu_ShowGameTitle(void)
{
	static vguiMenuTitle gameTitle;

	if (!gameTitle) {
		gameTitle = spawn(vguiMenuTitle);
		Menu_Add(gameTitle);
	}
}

static bool
Menu_ReserveRow(void)
{
	if (g_menuButtonCount >= MENUKIT_MAXBUTTONS) {
		ncError("More than %d main-menu rows, ignoring the rest.", MENUKIT_MAXBUTTONS);
		return (false);
	}

	return (true);
}

vguiMenuButton
Menu_AddButton(string title, string icon, void(void) func, menuVisibility_t visibility)
{
	vguiMenuButton newButton;

	if (Menu_ReserveRow() == false) {
		return (__NULL__);
	}

	newButton = spawn(vguiMenuButton);
	newButton.SetTitle(title);
	newButton.SetSize([128, 24]);
	newButton.SetFunc(func);

	if (STRING_SET(icon)) {
		newButton.SetIcon(icon);
	}

	/* vguiMenuButton starts out visible everywhere */
	if (!(visibility & MENU_OFFLINE)) {
		newButton.FlagRemove(MBUTTON_SHOWOFFLINE);
	}
	if (!(visibility & MENU_INGAME)) {
		newButton.FlagRemove(MBUTTON_SHOWSP | MBUTTON_SHOWMP);
	}

	g_menuButtons[g_menuButtonCount] = newButton;
	g_menuButtonVis[g_menuButtonCount] = visibility;
	g_menuButtonCount++;
	g_menuLayoutState = -1;

	Menu_Add(newButton);
	return (newButton);
}

void
Menu_AddSpacer(void)
{
	if (Menu_ReserveRow() == false) {
		return;
	}

	g_menuButtons[g_menuButtonCount] = __NULL__;
	g_menuButtonVis[g_menuButtonCount] = MENU_ALWAYS;
	g_menuButtonCount++;
	g_menuLayoutState = -1;
}

/* Stack the buttons that are visible right now, so hidden ones leave no gap. */
static void
Menu_LayoutButtons(void)
{
	menuVisibility_t currentState = Menu_InGame() ? MENU_INGAME : MENU_OFFLINE;
	int row = 1;

	if (g_menuLayoutState == (int)currentState) {
		return;
	}

	g_menuLayoutState = (int)currentState;

	for (int i = 0; i < g_menuButtonCount; i++) {
		if (!(g_menuButtonVis[i] & currentState)) {
			continue;
		}

		if (g_menuButtons[i]) {
			g_menuButtons[i].SetPosition(row);
		}

		row++;
	}
}

/* stock actions ----------------------------------------------------------- */

void
Menu_Resume(void)
{
	m_toggle(0);
}

void
Menu_Disconnect(void)
{
	localcmd("disconnect\n");
}

void
Menu_ShowConsole(void)
{
	static VGUIConsole consoleWindow;

	if (!consoleWindow) {
		consoleWindow = spawn(VGUIConsole);
	}

	Menu_Add(consoleWindow);
	consoleWindow.Show();
}

void
Menu_ShowQuitDialog(void)
{
	static vguiWindow winQuitGame;
	static vguiLabel lblSure;
	static vguiButton btnQuit;
	static vguiButton btnCancel;

	static void QuitGame_Yes(void) {
		winQuitGame.Hide();
		localcmd("quit\n");
	}
	static void QuitGame_No(void) {
		winQuitGame.Hide();
	}

	if (!winQuitGame) {
		winQuitGame = spawn(vguiWindow);
		winQuitGame.SetTitle("Quit Game");
		winQuitGame.SetSize([256, 128]);
		winQuitGame.SetIcon("gfx/icon16/stop");

		btnQuit = spawn(vguiButton);
		btnQuit.SetTitle("Quit");
		btnQuit.SetSize([64, 24]);
		btnQuit.SetPos(winQuitGame.GetSize() - [152, 32]);
		btnQuit.SetFunc(QuitGame_Yes);

		btnCancel = spawn(vguiButton);
		btnCancel.SetTitle("Cancel");
		btnCancel.SetSize([64, 24]);
		btnCancel.SetPos(winQuitGame.GetSize() - [80, 32]);
		btnCancel.SetFunc(QuitGame_No);

		lblSure = spawn(vguiLabel);
		lblSure.SetTitle("Do you wish to stop playing now?");
		lblSure.SetSize([256, 16]);
		lblSure.SetPos([0, 48]);
		lblSure.SetAlignment(AF_NONE);

		winQuitGame.Add(btnQuit);
		winQuitGame.Add(btnCancel);
		winQuitGame.Add(lblSure);
	}

	Menu_ShowCentered(winQuitGame);
}

/* engine entry points ------------------------------------------------------- */

void
m_init(void)
{
	Platform_Init();
	UISystem_Init();

	g_menuDesktop = spawn(vguiWidget);
	g_menuDesktop.FlagAdd(1);

	registercommand("console");
	registercommand("menu_quit");

	Game_MenuInit();
}

void
Menu_RendererRestarted(string renderer)
{
	Platform_RendererRestarted();
}

void
m_shutdown(void)
{
	Platform_Shutdown();
}

static void
Menu_CheckCanvas(vector screenSize)
{
	/* set new scale whenever the resolution changes */
	if (Platform_DimensionsChanged(screenSize) == true) {
		Platform_CalculateRenderingScale(screenSize);
		g_menuDesktop.Reposition();
		g_menuLayoutState = -1;
	}
}

void
m_draw(vector screenSize)
{
	Menu_CheckCanvas(screenSize);

	frametime = time - lasttime;
	lasttime = time;

	if (Platform_Draw(screenSize) == false) {
		return;
	}

	Menu_LayoutButtons();
	Game_MenuDraw(g_vidsize);
	g_menuDesktop.Draw();
}

void
m_drawloading(vector screenSize, float isOpaque)
{
	Menu_CheckCanvas(screenSize);
	g_vidsize = screenSize;
	Game_MenuLoading(screenSize, isOpaque);
}

float
Menu_InputEvent(float evtype, float scanx, float chary, float devid)
{
	switch (evtype) {
	case IE_KEYDOWN:
		if (chary == K_ESCAPE && Menu_InGame() == true) {
			m_toggle(0);
		}
		break;
	case IE_MOUSEABS:
		g_vecMousePos[0] = scanx;
		g_vecMousePos[1] = chary;
		break;
	case IE_MOUSEDELTA:
		g_vecMousePos[0] += scanx;
		g_vecMousePos[1] += chary;
		break;
	}

	g_vecMousePos = getmousepos();
	g_menuDesktop.Input(evtype, scanx, chary, devid);
	return (float)g_active;
}

void
m_display(void)
{
	g_active = TRUE;
	setkeydest(KEY_MENU);
	setmousetarget(TARGET_MENU);
	setcursormode(TRUE, "gfx/icon16/cursor");
}

void
m_hide(void)
{
	g_active = FALSE;
	setkeydest(KEY_GAME);
	setmousetarget(TARGET_CLIENT);
	setcursormode(FALSE);
}

void
m_toggle(float fMode)
{
	if (fMode == FALSE)
		m_hide();
	else
		m_display();
}

float
m_consolecommand(string cmd)
{
	int c = (int)tokenize(cmd);

	switch (argv(0)) {
	case "console":
		Menu_ShowConsole();
		break;
	case "menu_quit":
		Menu_ShowQuitDialog();
		break;
	default:
		if (Game_MenuConsoleCommand(argv(0), c) == true) {
			return (1);
		}

		/* the game may have tokenized something else */
		c = (int)tokenize(cmd);
		return Platform_ConsoleCommand(cmd, c);
	}

	return (1);
}
