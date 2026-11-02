//
//	sh_defs.qc - constants shared by client and server, included in both .src
//

// Custom stats (STAT_USER and up): registered with clientstat in worldspawn,
// read with getstatf in csqc.
const float STAT_JUMPS = STAT_USER;		// how many times the player jumped, for the hud

// The player's size and eye height (quake's).
const vector PLAYER_MINS = '-16 -16 -24';
const vector PLAYER_MAXS = '16 16 32';
const vector PLAYER_VIEW_OFS = '0 0 22';
