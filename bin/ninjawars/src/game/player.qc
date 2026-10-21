//
//	player.qc - player spawning and the per-client entry points called by the engine
//

// Map marker. PutClientInServer looks it up, so it must not remove itself.
void() info_player_start =
{
};

entity() find_spawn_point =
{
	entity spot = find(world, classname, "info_player_start");
	if (!spot)
		print("WARNING: map has no info_player_start, spawning at the origin\n");
	return spot;
};

void() PlayerSpawn =
{
	entity spot = find_spawn_point();

	self.classname = "player";
	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_WALK;
	self.health = 100;
	self.velocity = '0 0 0';
	setmodel(self, "models/testplayer.iqm");
	// 32x32x56 player hull with the origin at the feet, same as info_player_start in ninjawars.fgd.
	setsize(self, [-16, -16, 0], [16, 16, 56]);
	// +1 so the feet don't start embedded in the floor.
	setorigin(self, spot.origin + '0 0 1');
	self.view_ofs = [0, 0, 48];
	// fixangle makes the client take these angles instead of keeping its own.
	self.angles = spot.angles;
	self.fixangle = TRUE;
};

void() CheckImpulses =
{
	switch (self.impulse)
	{
	default: break;
	}
	self.impulse = 0;
};

// Jumping is handled by runstandardplayerphysics (it tracks the held jump button itself).
void() PlayerPreThink =
{
	CheckImpulses();
};

void() PlayerPostThink =
{
};

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
