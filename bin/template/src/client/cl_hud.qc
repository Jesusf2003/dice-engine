//
//	cl_hud.qc - the hud csqc draws over the 3d view:
//
//	-	the player's stats (health, the jumps the server counts) and position
//	-	the last messages printed to this player (sprint, bprint), fading out
//	-	centerprints, in the middle of the screen
//

#define HUD_MESSAGES		5		// lines kept
#define HUD_MESSAGE_TIME	6		// seconds a line stays
#define HUD_CENTER_TIME		5

const vector HUD_FONT = '12 12 0';
const vector HUD_BIG_FONT = '20 20 0';

string	hud_message[HUD_MESSAGES];		// [0] is the newest
float	hud_message_time[HUD_MESSAGES];
string	hud_center;
float	hud_center_time;

void(string msg) Hud_AddMessage =
{
	msg = strtrim(msg);
	if (msg == "")
		return;
	for (float i = HUD_MESSAGES - 1; i > 0; i--)
	{
		hud_message[i] = hud_message[i - 1];
		hud_message_time[i] = hud_message_time[i - 1];
	}
	hud_message[0] = msg;
	hud_message_time[0] = time;
};

void(string msg) Hud_CenterPrint =
{
	hud_center = msg;
	hud_center_time = time;
};

// 1 while it's fresh, down to 0 over its last second
float(float shown, float duration) Hud_Fade =
{
	return bound(0, shown + duration - time, 1);
};

void(float y, string text, vector size, vector rgb, float alpha) Hud_DrawCentered =
{
	float w = stringwidth(text, TRUE, size);
	drawstring([(g_width - w) / 2, y], text, size, rgb, alpha, DRAWFLAG_NORMAL);
};

void(float vwidth, float vheight) Hud_Draw =
{
	// title
	drawstring('16 16 0', "Template", HUD_BIG_FONT, '1 0.8 0.3', 1, DRAWFLAG_NORMAL);

	// messages, under the title
	float y = 44;
	for (float i = 0; i < HUD_MESSAGES; i++)
	{
		float a = Hud_Fade(hud_message_time[i], HUD_MESSAGE_TIME);
		if (!hud_message[i] || a <= 0)
			continue;
		drawstring([16, y], hud_message[i], HUD_FONT, '1 1 1', a, DRAWFLAG_NORMAL);
		y += 16;
	}

	// centerprint: one line per \n
	float ca = Hud_Fade(hud_center_time, HUD_CENTER_TIME);
	if (hud_center && ca > 0)
	{
		float lines = tokenizebyseparator(hud_center, "\n");
		float cy = vheight * 0.3;
		for (float l = 0; l < lines; l++)
		{
			Hud_DrawCentered(cy, argv(l), HUD_BIG_FONT, '1 1 1', ca);
			cy += 26;
		}
	}

	// stats panel, bottom left
	vector panel = [16, vheight - 76];
	drawfill(panel, '220 60 0', '0 0 0', 0.45, DRAWFLAG_NORMAL);
	drawstring(panel + '10 10 0', sprintf("Health  %d", getstatf(STAT_HEALTH)), HUD_FONT, '0.6 1 0.6', 1, DRAWFLAG_NORMAL);
	drawstring(panel + '10 26 0', sprintf("Jumps   %d", getstatf(STAT_JUMPS)), HUD_FONT, '0.6 0.8 1', 1, DRAWFLAG_NORMAL);
	vector org = getproperty(VF_ORIGIN);
	drawstring(panel + '10 42 0', sprintf("Pos     %d %d %d", org_x, org_y, org_z), HUD_FONT, '0.8 0.8 0.8', 1, DRAWFLAG_NORMAL);
};
