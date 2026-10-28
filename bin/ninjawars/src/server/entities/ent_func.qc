//
//	ent_func.qc - brush entities. Only the spawn functions the map calls live here; behaviour that
//	needs more than a few lines goes in its own sv_*.qc module.
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

/* QUAKED func_door (0 .5 .8) ? START_OPEN
Brush that slides open when the player touches it, waits and closes again.
angle		direction it opens to (-1 up, -2 down)
distance	units it moves (default: its own size minus lip)
speed		units per second (default 100)
wait		seconds before closing (default 3, -1 = never closes)
lip			units left visible when open, only without distance (default 8)
targetname	if set, touching does nothing: only opens when triggered
noise1		sound while moving
noise2		sound when it stops closed
START_OPEN	starts at the open position and moves to where it was placed
*/
void() func_door =
{
	DoorSpawn();	// sv_doors.qc
};

/* QUAKED func_door_rotating (0 .5 .8) ? START_OPEN REVERSE x x x x X_AXIS Y_AXIS
Brush that swings open like a normal door. Needs an origin brush (all faces textured "origin") on the hinge:
that is the point it rotates around.
distance	degrees it rotates (default 90), seen from above positive is counter-clockwise
speed		degrees per second (default 100)
wait, targetname, noise1, noise2: same as func_door
REVERSE		rotate the other way
X_AXIS		hinge on X (rolls), Y_AXIS hinge on Y (pitches); default is a vertical hinge
*/
void() func_door_rotating =
{
	DoorRotatingSpawn();	// sv_doors.qc
};

/* QUAKED func_test (0 0 0) (-8 -8 -8) (8 8 8)
*/
void() func_test = {
  print("func_test triggered!\n");
};
