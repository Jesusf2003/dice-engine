//
//	main.qc - global server entry points called by the engine
//

// Required by the compiler, the engine never calls it.
void() main =
{
  precache_file("progs.dat");
  precache_file("default.cfg");
  precache_file("maps/start_demo.bsp");
};

void() precache_models =
{
  precache_model("models/testplayer.iqm");
};

// First entity of every map. Precache everything the map can use here.
void() worldspawn =
{
  precache_models();
	print("Welcome to Ninja Wars! :)\n");
};

// Called once per server frame, before any entity thinks.
void() StartFrame = {};