//
//	sv_player.qc - player spawning and per-frame player logic
//

info_player_start() find_spawn_point =
{
	info_player_start spot = (info_player_start)find(world, classname, "info_player_start");
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
	self.health = 600;
	self.velocity = '0 0 0';
	setmodel(self, "models/heritage_gun.iqm");
	setsize(self, [-16, -16, 0], [16, 16, 56]);
	setorigin(self, spot.origin + '0 0 1');
	self.view_ofs = [0, 0, 48];
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

void() PlayerPreThink =
{
	CheckImpulses();
};

void() PlayerPostThink =
{
};

