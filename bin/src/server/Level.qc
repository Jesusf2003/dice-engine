/*
 * Copyright (c) 2016-2024 Vera Visions LLC.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/


void
Level_PlayerReady(void)
{
	static bool called = false;

	/* don't do it more than once */
	if (called) {
		return;
	}

	for (entity e = world; (e = find(e, ::classname, "trigger_auto"));) {
		ncEntity autoTrigger = (ncEntity)e;
		autoTrigger.Input(world, "MapStart", "");
	}

	called = true;
}

vector
Landmark_GetSpot(void)
{
	/* a trigger_transition may share the same targetname, thus we do this */
	for (entity e = world; (e = find(e, ::classname, "info_landmark"));) {
		/* found it */
		if (e.targetname == startspot) {
			ncLog("^3Landmark_GetSpot^7: Found landmark for %s", startspot);
			return e.origin + g_landmarkpos;
		}
	}

	/* return something useful at least */
	entity ips = find(world, ::classname, "info_player_start");
	ncError("Cannot find startspot %S!\n", startspot);
	return ips.origin;
}

vector
Landmark_GetPosition(void)
{
	return g_landmarkpos;
}

void
ChangeTarget_Activate(void)
{
	string ctarg = cvar_string("_bsp_changetarget");
	ncTimer foo = __NULL__;

	static void Finalize(void) {
		string ctarg = cvar_string("_bsp_changetarget");

		if not (ctarg)
			return;

		if (ctarg == "")
			return;

		for (entity a = world; (a = find(a, ::targetname, ctarg));) {
			ncEntity t = (ncEntity)a;

			if (t.Trigger)
				t.Trigger(self, TRIG_TOGGLE);
		}

		cvar_set("_bsp_changetarget", "");
		cvar_set("_bsp_changedelay", "");
	}

	if not (ctarg)
		return;

	foo.TemporaryTimer(self, Finalize, cvar("_bsp_changedelay"), false);
}
