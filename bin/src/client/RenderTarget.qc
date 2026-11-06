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


var bool g_rtMonitorActive;
var float g_rtMonitorFOV;
var vector g_rtMonitorSize;
var vector g_rtMonitorPos;
var vector g_rtMonitorAngles;

void
RenderTarget_SetMonitor(bool active, vector pos, vector ang, float fov, vector size)
{
	g_rtMonitorActive = active;
	g_rtMonitorPos = pos;
	g_rtMonitorAngles = ang;
	g_rtMonitorFOV = fov;
	g_rtMonitorSize = size;
}

void
RenderTarget_UpdateMonitor(void)
{
	if (g_rtMonitorActive == false) {
		return;
	}

	clearscene();

	/* TODO: This is ideally where fog parms should be set... :/ */
	addentities(MASK_ENGINE);

	setproperty(VF_RT_DESTCOLOUR, "base", (float)1, g_rtMonitorSize);
	setproperty(VF_SIZE, g_rtMonitorSize);
	setproperty(VF_DRAWENGINESBAR, (float)0);
	setproperty(VF_ORIGIN, g_rtMonitorPos);
	setproperty(VF_ANGLES, g_rtMonitorAngles);
	setproperty(VF_AFOV, g_rtMonitorFOV);
	renderscene();
	setproperty(VF_RT_DESTCOLOUR, "");
}
