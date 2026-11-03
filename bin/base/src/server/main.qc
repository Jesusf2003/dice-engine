void(void) Game_Worldspawn = {}

/* Singleplayer only: always load progs/singleplayer.dat, whatever g_gametype or the slot count say. */
string Game_DefaultRules(void)
{
	return ("singleplayer");
}
