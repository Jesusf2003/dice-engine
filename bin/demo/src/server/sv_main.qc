/*
 * Engine entry points of the server game.
 * The engine calls these by name; every one must exist even when empty.
 */

void() main = {};

/* Runs once per server frame, before any entity thinks. */
void() StartFrame = {};

/* Save/restore of per-client values across map changes (parm1..parm16). Unused. */
void() SetNewParms = {};
void() SetChangeParms = {};

/* A client finished loading the map. PutClientInServer (sv_player.qc) is called right after. */
void() ClientConnect =
{
	bprint(self.netname, " entered the game\n");
};

void() ClientDisconnect = {};

/* "kill" console command: respawn at the start spot. */
void() ClientKill =
{
	PutClientInServer();
};

/* Around each player's movement (SV_RunClientCommand in sv_player.qc). Unused. */
void() PlayerPreThink = {};
void() PlayerPostThink = {};
