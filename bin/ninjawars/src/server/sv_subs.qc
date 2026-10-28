//
//	sv_subs.qc - generic helpers shared by entities. Movers: move a MOVETYPE_PUSH brush to a point
//	at constant speed (doors now, plats/buttons later).
//

void() SetMovedir =
{
	if (self.angles == '0 -1 0')
		self.movedir = '0 0 1';
	else if (self.angles == '0 -2 0')
		self.movedir = '0 0 -1';
	else
	{
		makevectors(self.angles);
		self.movedir = v_forward;
	}
	self.angles = '0 0 0';
};

void() SUB_CalcMoveDone =
{
	setorigin(self, self.finaldest);
	self.velocity = '0 0 0';
	if (self.think1)
		self.think1();
};

void(vector tdest, float tspeed, void() func) SUB_CalcMove =
{
	local vector delta;
	local float traveltime;

	self.think1 = func;
	self.finaldest = tdest;
	self.think = SUB_CalcMoveDone;

	delta = tdest - self.origin;
	traveltime = vlen(delta) / tspeed;
	if (traveltime < 0.1)
	{
		self.velocity = '0 0 0';
		self.nextthink = self.ltime + 0.1;
		return;
	}
	self.velocity = delta * (1 / traveltime);
	self.nextthink = self.ltime + traveltime;
};

void() SUB_CalcAngleMoveDone =
{
	self.angles = self.finalangle;
	self.avelocity = '0 0 0';
	if (self.think1)
		self.think1();
};

void(vector tdest, float tspeed, void() func) SUB_CalcAngleMove =
{
	local vector delta;
	local float traveltime;

	self.think1 = func;
	self.finalangle = tdest;
	self.think = SUB_CalcAngleMoveDone;

	delta = tdest - self.angles;
	traveltime = vlen(delta) / tspeed;
	if (traveltime < 0.1)
	{
		self.avelocity = '0 0 0';
		self.nextthink = self.ltime + 0.1;
		return;
	}
	self.avelocity = delta * (1 / traveltime);
	self.nextthink = self.ltime + traveltime;
};
