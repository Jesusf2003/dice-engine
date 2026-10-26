//
//	ent_func.qc - brush entities
//

void() func_illusionary =
{
	setmodel(self, self.model);
	self.solid = SOLID_NOT;
};

/* QUAKED func_ladder (1 .8 0) ?
Invisible climbable volume. Place it against a wall: the player climbs while inside it and facing the wall.
Move forward looking up/down to climb, or hold jump to go up.
*/
void() func_ladder =
{
	self.solid = SOLID_BSP;
	self.movetype = MOVETYPE_PUSH;
	self.skin = CONTENT_LADDER;
	setmodel(self, self.model);
	self.effects |= EF_NODRAW;
};

/* QUAKED func_test (0 0 0) (-8 -8 -8) (8 8 8)
*/
void() func_test = {
  print("func_test triggered!\n");
};