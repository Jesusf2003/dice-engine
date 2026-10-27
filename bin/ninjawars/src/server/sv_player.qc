//
//	sv_player.qc - player spawning and per-frame player logic
//

// models/marine!.spr frames. The angled ones pick 1 of 8 directions from the camera by themselves.
// The ! in the name makes FTE light it like a model (darker in shadows) instead of drawing it fullbright.
const float MARINE_WALK = 0;	// 0-3, angled
const float MARINE_ATTACK = 4;	// angled
const float MARINE_FIRE = 5;	// angled, muzzle flash
const float MARINE_PAIN = 6;	// angled
const float MARINE_DEATH = 7;	// 7-13
const float MARINE_GIB = 14;	// 14-22

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
	self.health = 100;
	self.velocity = '0 0 0';

	setmodel(self, "models/sprites/marine!.spr");
	self.scale = 1;	// the sprite is 1 pixel per unit, with its feet on mins_z
	self.frame = MARINE_WALK;
	setsize(self, [-16, -16, -16], [16, 16, 40]);
	setorigin(self, spot.origin + '0 0 17');

	self.view_ofs = [0, 0, 32];
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

void() PlayerAnimate =
{
	if (vlen([self.velocity_x, self.velocity_y, 0]) > 20)
		self.frame = MARINE_WALK + floor(time * 8) % 4;
	else
		self.frame = MARINE_WALK;
};

void() PlayerPostThink =
{
	PlayerAnimate();
};

