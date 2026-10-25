//
//	ent_func.qc - brush entities
//

void() func_illusionary =
{
	setmodel(self, self.model);
	self.solid = SOLID_NOT;
};
