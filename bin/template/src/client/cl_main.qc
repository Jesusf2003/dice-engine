//
//	cl_main.qc - every CSQC entry point called by the engine. The view is the engine's
//	own first person one: clearscene() places it at the player's eyes.
//

void(float apilevel, string enginename, float engineversion) CSQC_Init =
{
	// Console commands csqc handles, in CSQC_ConsoleCommand.
	registercommand("hud_say", "Shows a message on the hud: hud_say <text>");
	print("Template client loaded\n");
};

float(string cmd) CSQC_ConsoleCommand =
{
	tokenize_console(cmd);
	switch (argv(0))
	{
	case "hud_say":
		string msg = substring(cmd, strlen("hud_say "), -1);
		print("hud_say: ", msg, "\n");	// the console
		Hud_AddMessage(msg);			// the hud
		return TRUE;
	}
	return FALSE;
};

// The server's sprint and bprint: to the console as usual, and to the hud.
void(string printmsg, float printlvl) CSQC_Parse_Print =
{
	print(printmsg);
	Hud_AddMessage(printmsg);
};

// The server's centerprint: drawn by the hud instead of the engine (TRUE).
float(string msg) CSQC_Parse_CenterPrint =
{
	Hud_CenterPrint(msg);
	return TRUE;
};

// Return TRUE to eat the event so the engine doesn't run binds for it.
float(float evtype, float scanx, float chary, float devid) CSQC_InputEvent =
{
	return FALSE;
};

// fteextensions.qc marks it __used, so it must have a body. The engine's input
// (the +forward, +jump... binds in default.cfg) goes to the server as it is.
void() CSQC_Input_Frame =
{
};

void(float vwidth, float vheight, float notmenu) CSQC_UpdateView =
{
	g_width = vwidth;
	g_height = vheight;

	clearscene();							// the view at the player's eyes, the engine's defaults
	addentities(MASK_ENGINE | MASK_VIEWMODEL);	// the map's entities and the weapon, if any
	setproperty(VF_DRAWENGINESBAR, FALSE);	// csqc draws the hud
	setproperty(VF_DRAWCROSSHAIR, TRUE);
	renderscene();

	Hud_Draw(vwidth, vheight);
};
