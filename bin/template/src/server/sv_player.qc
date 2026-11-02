//
//	sv_player.qc - the player: spawning, and noticing its jumps
//

entity() FindSpawnPoint =
{
	entity spot = find(world, classname, "info_player_start");
	if (!spot)
		print("WARNING: the map has no info_player_start, spawning at the origin\n");
	return spot;
};

void() PlayerSpawn =
{
	entity spot = FindSpawnPoint();

	self.classname = "player";
	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_WALK;
	self.takedamage = DAMAGE_NO;
	self.health = 100;
	self.flags = FL_CLIENT;
	self.velocity = '0 0 0';
	self.jumps = 0;

	// No model: the view is the player's eyes, nothing of it shows in first person.
	setsize(self, PLAYER_MINS, PLAYER_MAXS);
	setorigin(self, spot.origin + '0 0 1');
	self.view_ofs = PLAYER_VIEW_OFS;
	self.angles = spot.angles;
	self.fixangle = TRUE;		// the client takes these angles instead of keeping its own

	// Messages: sprint goes to this player's console (csqc shows it on the hud too),
	// centerprint to the middle of its screen.
	sprint(self, "Welcome to the template! WASD to move, SPACE to jump.\n");
	centerprint(self, "Template\nWASD to move, SPACE to jump");
};

// Called around the movement of every client input frame, from SV_RunClientCommand.
void() PlayerPreMove =
{
	self.was_onground = self.flags & FL_ONGROUND;
};

void() PlayerPostMove =
{
	// Left the ground going up: a jump.
	if (self.was_onground && !(self.flags & FL_ONGROUND) && self.velocity_z > 0)
	{
		self.jumps = self.jumps + 1;
		sprint(self, sprintf("Jump %d\n", self.jumps));
		if (self.jumps == 10)
			centerprint(self, "10 jumps!");
	}
};
