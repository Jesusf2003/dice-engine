//
//	main.qc - global server entry points called by the engine
//

// Required by the compiler, the engine never calls it.
void() main =
{
};

void() precache_models =
{
  precache_model("models/testplayer.iqm");
};

// First entity of every map. Precache everything the map can use here.
void() worldspawn =
{
	cvar_set("sv_sound_land", "");
  precache_models();
	print("Welcome to Ninja Wars!\n");
};

// Called once per server frame, before any entity thinks.
void() StartFrame =
{
};
