/*
 * Client game (CSQC), "simple CSQC" mode: since CSQC_UpdateView is not
 * defined, the engine draws the 3D view and predicts the player's movement
 * itself, and only calls CSQC_DrawHud to draw the 2D overlay.
 * Define CSQC_UpdateView to take over rendering (camera, view model, HUD).
 */

void(float apilevel, string enginename, float engineversion) CSQC_Init = {};

/* Runs before each input frame is sent to the server; the input_* globals can be edited here. */
void() CSQC_Input_Frame = {};

void(vector viewsize, float scoresshown) CSQC_DrawHud =
{
	/* Bottom-left corner: the top lines belong to the console's notify messages. */
	drawstring([8, viewsize_y - 16], "demo - WASD: move, SPACE: jump, mouse: look", '8 8 0', '1 1 1', 1, DRAWFLAG_NORMAL);
};
