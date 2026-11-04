/*
 * Copyright (c) 2025 Vera Visions LLC
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

/* Server-side debug drawing. The engine calls SV_AddDebugPolygons each video frame
   when it exists, and only with a local client (singleplayer / listen server).
   - nav_visualize 1: the navigation nodes and links
   - r_renderEntityInfo 1: each entity's DebugDraw() (bounding box, paths, ...)
   - developer 1: loads the map's navmesh for the above if it isn't yet */

var bool autocvar_r_renderEntityInfo = false;
var bool autocvar_nav_visualize = false;
void
SV_AddDebugPolygons(void)
{
	if (autocvar_nav_visualize) {
		ncNodeEditor::DrawDebugInfo();
	}

	if (autocvar_r_renderEntityInfo) {
		makevectors(self.v_angle);

		for (entity s = world; (s = findfloat(s, ::identity, 1));) {
			ncEntity drawMe = (ncEntity)s;

			/* identity is set by ncIO: dictionaries etc. have no DebugDraw */
			if (drawMe.DebugDraw) {
				drawMe.DebugDraw();
			}
		}
	}

	if (cvar("developer") != 1)
		return;

	if (!g_nodeCount) {
		ncNodeEditor::LoadCurrentMapNavMesh();
	}
}
