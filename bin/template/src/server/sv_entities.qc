//
//	sv_entities.qc - the map's entities. Each classname in the map runs the function
//	of the same name when the map loads; one without a function is removed with a warning.
//

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
Where the player starts. "angle" is the direction it faces.
*/
void() info_player_start =
{
};

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8)
A light. The map compiler (or the engine's realtime lighting, for a .map loaded
as it is) reads it from the map: the game doesn't need it.
"light" brightness (300), "_color" "r g b" from 0 to 1.
*/
void() light =
{
	remove(self);
};
