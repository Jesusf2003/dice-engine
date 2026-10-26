//
//	sv_main.qc - every server entry point called by the engine
//

void() main =
{
  precache_file("progs.dat");
  precache_file("default.cfg");
  precache_file("maps/start_demo.bsp");
};

void() precache_models =
{
  precache_model("models/player_test.iqm");
  precache_model("models/weapons/v_pistol.iqm");
};

void() worldspawn =
{
    precache_models();
	clientstat(STAT_WEAPONSHOT, EV_FLOAT, weaponshot);
	print("Welcome to Ninja Wars! :)\n");
};

void() StartFrame = {};

void() SV_RunClientCommand =
{
	runstandardplayerphysics(self);
};

void() ClientConnect =
{
};

void() PutClientInServer =
{
	PlayerSpawn();
};

void() ClientKill =
{
};

void() ClientDisconnect =
{
	bprint(self.netname, " has left the game.\n");
};

void() SetNewParms =
{
};

void() SetChangeParms =
{
};
