/*
 * The player: spawning and movement.
 */

/* Places the client's entity at the map's info_player_start. */
void() PutClientInServer =
{
	entity spot = find(world, classname, "info_player_start");
	if (!spot)
		spot = world;

	self.classname = "player";
	self.health = 100;
	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_WALK;
	self.flags = FL_CLIENT;

	/* First person: the player has no visible model, only a collision box. */
	setorigin(self, spot.origin + '0 0 1');
	setsize(self, PLAYER_MINS, PLAYER_MAXS);
	self.view_ofs = PLAYER_VIEW_OFS;
	self.velocity = '0 0 0';

	self.angles = spot.angles;
	self.fixangle = TRUE;
};

/*
 * Called for every movement packet the client sends; the input_* globals hold it.
 * runstandardplayerphysics is the engine's own player movement (walking, gravity,
 * jumping with +jump, stairs, friction). The client predicts with the same code,
 * so movement stays smooth.
 */
void() SV_RunClientCommand =
{
	float wasonground = self.flags & FL_ONGROUND;

	runstandardplayerphysics(self);

	if (wasonground && !(self.flags & FL_ONGROUND) && (input_buttons & INPUT_BUTTON_JUMP) && self.velocity_z > 0)
		sound(self, CHAN_BODY, SND_JUMP, 1, ATTN_NORM);
};
