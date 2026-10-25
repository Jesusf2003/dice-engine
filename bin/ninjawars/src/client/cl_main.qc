//
//	cl_main.qc - every CSQC entry point called by the engine. Each one only hands the work
//	to the module that owns it, so new modules don't fight over the same function.
//

void(float apilevel, string enginename, float engineversion) CSQC_Init =
{
	// registercommand("name") here, then handle it in CSQC_ConsoleCommand.
};

// Commands registered in CSQC_Init.
float(string cmd) CSQC_ConsoleCommand =
{
	tokenize_console(cmd);
	return FALSE;
};

// Return TRUE to eat the event so the engine doesn't run binds for it.
float(float evtype, float scanx, float chary, float devid) CSQC_InputEvent =
{
	return FALSE;
};

// fteextensions.qc marks it __used, so it must have a body.
void() CSQC_Input_Frame =
{
	Input_Frame();
};

void(float vwidth, float vheight, float notmenu) CSQC_UpdateView =
{
	g_width = vwidth;
	g_height = vheight;

	clearscene();
	Cam_SetupView();

	// The engine still draws its status bar and crosshair under the csqc hud.
	setproperty(VF_DRAWENGINESBAR, TRUE);
	setproperty(VF_DRAWCROSSHAIR, TRUE);
	renderscene();

	Hud_Draw(vwidth, vheight);
};
