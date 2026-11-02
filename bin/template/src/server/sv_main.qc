//
//	sv_main.qc - every server entry point called by the engine
//

void() main =
{
};

// The first entity of every map: precaches and globals go here.
void() worldspawn =
{
	clientstat(STAT_JUMPS, EV_FLOAT, jumps);
	print("Template: map ", mapname, " loaded\n");	// the server's console
};

void() StartFrame = {};

// Every input frame of every client. The engine's movement (walk, jump, step,
// gravity), with the jump check around it.
void() SV_RunClientCommand =
{
	PlayerPreMove();
	runstandardplayerphysics(self);
	PlayerPostMove();
};

void() ClientConnect =
{
	bprint(self.netname, " entered the game\n");	// every player's console
};

void() PutClientInServer =
{
	PlayerSpawn();
};

void() ClientKill =
{
	PlayerSpawn();		// "kill" in the console: back to the start
};

void() ClientDisconnect =
{
	bprint(self.netname, " left the game\n");
};

void() SetNewParms = {};
void() SetChangeParms = {};

// Called by the engine once per server frame around the player's physics; the
// template does its per-move work in SV_RunClientCommand instead.
void() PlayerPreThink = {};
void() PlayerPostThink = {};
