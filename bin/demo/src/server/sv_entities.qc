/*
 * Spawn functions: the engine calls the function named after each map
 * entity's "classname" when the map loads. Entities without one are removed
 * with a warning.
 */

/* The world itself (entity 0). Precache every sound/model the game uses here. */
void() worldspawn =
{
	precache_sound(SND_JUMP);
};

/* Where players spawn; PutClientInServer looks it up by classname. */
void() info_player_start = {};

/* Point light. vmap -light bakes it into the map's lightmaps, so it is not needed at runtime. */
void() light =
{
	remove(self);
};
