//
//	ent_func.qc - brush entities
//

void() func_illusionary =
{
	setmodel(self, self.model);
	self.solid = SOLID_NOT;
};

/* QUAKED func_test (0 0 0) (-8 -8 -8) (8 8 8)
*/
void() func_test = {
  print("func_test triggered!\n");
};