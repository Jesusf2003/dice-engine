//
//	ent_light.qc - light (see ninjawars.fgd)
//

// light.exe bakes it into the lightmap when compiling the map, it's not needed at runtime.
void() light =
{
	remove(self);
};
