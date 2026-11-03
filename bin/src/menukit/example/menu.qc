/* MenuKit example: a minimal singleplayer menu built only from the MenuKit API and VGUI.
   Copy this folder to <game>/src/menu/, fix GAME_DIR and the include path in progs.src. */

/* New Game: a small window with a difficulty choice */
void
Example_NewGame(void)
{
	static vguiWindow winNewGame;
	static vguiRadio radEasy;
	static vguiRadio radMedium;
	static vguiRadio radHard;
	static vguiButton btnPlay;
	static vguiButton btnCancel;

	static void NewGame_Uncheck(void) {
		radEasy.SetValue(false);
		radMedium.SetValue(false);
		radHard.SetValue(false);
	}
	static void NewGame_Easy(void) { NewGame_Uncheck(); radEasy.SetValue(true); }
	static void NewGame_Medium(void) { NewGame_Uncheck(); radMedium.SetValue(true); }
	static void NewGame_Hard(void) { NewGame_Uncheck(); radHard.SetValue(true); }

	static void NewGame_Play(void) {
		float skillValue = 2;

		if (radEasy.GetValue())
			skillValue = 1;
		else if (radHard.GetValue())
			skillValue = 3;

		winNewGame.Hide();
		Map_NewGame(skillValue);
	}
	static void NewGame_Cancel(void) {
		winNewGame.Hide();
	}

	if (!winNewGame) {
		winNewGame = spawn(vguiWindow);
		winNewGame.SetTitle("New Game");
		winNewGame.SetSize([256, 160]);
		winNewGame.SetIcon("gfx/icon16/new");

		radEasy = spawn(vguiRadio);
		radEasy.SetTitle("Easy");
		radEasy.SetPos([16, 40]);
		radEasy.SetFunc(NewGame_Easy);

		radMedium = spawn(vguiRadio);
		radMedium.SetTitle("Medium");
		radMedium.SetPos([16, 64]);
		radMedium.SetFunc(NewGame_Medium);
		radMedium.SetValue(true);

		radHard = spawn(vguiRadio);
		radHard.SetTitle("Hard");
		radHard.SetPos([16, 88]);
		radHard.SetFunc(NewGame_Hard);

		btnPlay = spawn(vguiButton);
		btnPlay.SetTitle("Play");
		btnPlay.SetSize([64, 24]);
		btnPlay.SetPos(winNewGame.GetSize() - [152, 32]);
		btnPlay.SetFunc(NewGame_Play);

		btnCancel = spawn(vguiButton);
		btnCancel.SetTitle("Cancel");
		btnCancel.SetSize([64, 24]);
		btnCancel.SetPos(winNewGame.GetSize() - [80, 32]);
		btnCancel.SetFunc(NewGame_Cancel);

		winNewGame.Add(radEasy);
		winNewGame.Add(radMedium);
		winNewGame.Add(radHard);
		winNewGame.Add(btnPlay);
		winNewGame.Add(btnCancel);
	}

	Menu_ShowCentered(winNewGame);
}

void
Game_MenuInit(void)
{
	Menu_ShowGameTitle();

	Menu_AddButton("Resume Game", "gfx/icon16/control_play", Menu_Resume, MENU_INGAME);
	Menu_AddButton("New Game", "gfx/icon16/new", Example_NewGame, MENU_ALWAYS);
	Menu_AddButton("Console", "gfx/icon16/application_xp_terminal", Menu_ShowConsole, MENU_ALWAYS);
	Menu_AddSpacer();
	Menu_AddButton("Disconnect", "gfx/icon16/disconnect", Menu_Disconnect, MENU_INGAME);
	Menu_AddButton("Quit", "gfx/icon16/stop", Menu_ShowQuitDialog, MENU_ALWAYS);
}
