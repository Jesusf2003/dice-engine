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


static entity g_cubemapCycle;

static void Cubemaps_Next(void);

/* renders the probe in self.owner, or finishes when there are none left */
static void
Cubemaps_Shoot(void)
{
	entity probe = self.owner;

	if (!probe) {
		ncLog("Generating level cubemaps done.");
		localcmd("r_skipEnvmap 0\n");
		localcmd("vid_reload\n");
		g_cubemapBuilding = false;
		remove(self);
		return;
	}

	g_cubemapPos = probe.origin;
	g_cubemapSize = (probe.cubemapSize > 0) ? probe.cubemapSize : 32;
	ncLog("Generating cubemap '%v' (%dx%d)", g_cubemapPos, g_cubemapSize, g_cubemapSize);

	g_view.SetClientAngle([0,0,0]);
	g_view.SetCameraAngle([0,0,0]);

	string fileName = sprintf("env/%s_%d_%d_%d.ktx", mapname, g_cubemapPos[0], g_cubemapPos[1], g_cubemapPos[2]);
	localcmd(sprintf("screenshot_cubemap %S %i\n", fileName, (int)g_cubemapSize));

	self.think = Cubemaps_Next;
	self.nextthink = time + 0.25f;
}

/* moves on to the next probe once the screenshot was dumped */
static void
Cubemaps_Next(void)
{
	self.owner = find(self.owner, classname, "env_cubemap");
	self.think = Cubemaps_Shoot;
	self.nextthink = time + 0.1f; /* because sigh */
}

void
Cubemaps_Build(void)
{
	/* prevent command spam */
	if (g_cubemapBuilding == true) {
		return;
	}

	cvar_set("r_speeds", "0");
	cvar_set("show_fps", "0");
	cvar_set("r_showfields", "0");
	cvar_set("r_showshaders", "0");
	cvar_set("r_showbboxes", "0");
	cvar_set("r_wireframe", "0");
	cvar_set("r_hdr_irisadaptation", "0");
	cvar_set("r_postprocshader", "\"\"0");
	cvar_set("r_skipEnvmap", "1");

	ncLog("Building cubemaps...\n");
	g_cubemapCycle = spawn();
	g_cubemapCycle.owner = find(world, classname, "env_cubemap");

	if (g_cubemapCycle.owner) {
		g_cubemapCycle.think = Cubemaps_Shoot;
		g_cubemapCycle.nextthink = time;
		g_cubemapBuilding = true;
	} else {
		remove(g_cubemapCycle);
	}
}
