//
//	sh_defs.qc - constants shared by client and server, included in both .src
//

// Custom stats (STAT_USER and up), registered with clientstat in worldspawn and read with getstatf in csqc.
const float STAT_PMOVE_FLAGS = STAT_USER;	// the player's .pmove_flags, so csqc knows when it is on a ladder

// The player's size: setsize on the server, the camera library's floor checks on the client.
const vector PLAYER_MINS = '-16 -16 -16';
const vector PLAYER_MAXS = '16 16 40';
