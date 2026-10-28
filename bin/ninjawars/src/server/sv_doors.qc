//
//	sv_doors.qc - door behaviour, sliding (func_door) and rotating (func_door_rotating). The map entities
//	live in ent_func.qc and only call DoorSpawn / DoorRotatingSpawn.
//
//	BOTTOM --touch/use--> UP --arrives--> TOP --wait--> DOWN --arrives--> BOTTOM
//	blocked while moving: go back the other way instead of crushing
//
//	Both kinds share this state machine: pos1 (closed) and pos2 (open) are an origin for sliding doors
//	and angles for rotating ones, and .mover is the SUB_ function that knows how to reach them.
//

void() DoorHitTop =
{
	self.state = STATE_TOP;
	if (self.wait < 0)
		return;
	self.think = DoorGoDown;
	self.nextthink = self.ltime + self.wait;
};

void() DoorHitBottom =
{
	self.state = STATE_BOTTOM;
	if (self.noise2 != "")
		sound(self, CHAN_VOICE, self.noise2, 1, ATTN_NORM);
};

void() DoorGoDown =
{
	self.state = STATE_DOWN;
	if (self.noise1 != "")
		sound(self, CHAN_VOICE, self.noise1, 1, ATTN_NORM);
	self.mover(self.pos1, self.speed, DoorHitBottom);
};

void() DoorGoUp =
{
	if (self.state == STATE_UP)
		return;
	if (self.state == STATE_TOP)
	{
		if (self.wait >= 0)
			self.nextthink = self.ltime + self.wait;
		return;
	}
	self.state = STATE_UP;
	if (self.noise1 != "")
		sound(self, CHAN_VOICE, self.noise1, 1, ATTN_NORM);
	self.mover(self.pos2, self.speed, DoorHitTop);
};

void() DoorUse =
{
	DoorGoUp();
};

void() DoorTouch =
{
	if (other.classname != "player")
		return;
	if (self.targetname != "")
		return;
	DoorGoUp();
};

void() DoorBlocked =
{
	if (self.state == STATE_DOWN)
		DoorGoUp();
	else if (self.state == STATE_UP)
		DoorGoDown();
};

void() DoorSetup =
{
	self.solid = SOLID_BSP;
	self.movetype = MOVETYPE_PUSH;
	setmodel(self, self.model);
	setorigin(self, self.origin);

	if (!self.speed)
		self.speed = 100;
	if (!self.wait)
		self.wait = 3;
	if (self.noise1 != "")
		precache_sound(self.noise1);
	if (self.noise2 != "")
		precache_sound(self.noise2);

	self.state = STATE_BOTTOM;
	self.touch = DoorTouch;
	self.use = DoorUse;
	self.blocked = DoorBlocked;
};

void() DoorSpawn =
{
	SetMovedir();
	DoorSetup();
	if (!self.lip)
		self.lip = 8;

	self.mover = SUB_CalcMove;
	self.pos1 = self.origin;
	if (self.distance)
		self.pos2 = self.pos1 + self.movedir * self.distance;
	else
		self.pos2 = self.pos1 + self.movedir * (fabs(self.movedir * self.size) - self.lip);

	if (self.spawnflags & DOOR_START_OPEN)
	{
		setorigin(self, self.pos2);
		self.pos2 = self.pos1;
		self.pos1 = self.origin;
	}
};

void() DoorRotatingSpawn =
{
	if (self.spawnflags & DOOR_X_AXIS)
		self.movedir = '0 0 1';
	else if (self.spawnflags & DOOR_Y_AXIS)
		self.movedir = '1 0 0';
	else
		self.movedir = '0 1 0';
	if (self.spawnflags & DOOR_REVERSE)
		self.movedir = self.movedir * -1;

	DoorSetup();
	if (!self.distance)
		self.distance = 90;

	self.mover = SUB_CalcAngleMove;
	self.pos1 = self.angles;
	self.pos2 = self.pos1 + self.movedir * self.distance;

	if (self.spawnflags & DOOR_START_OPEN)
	{
		self.angles = self.pos2;
		self.pos2 = self.pos1;
		self.pos1 = self.angles;
	}
};
