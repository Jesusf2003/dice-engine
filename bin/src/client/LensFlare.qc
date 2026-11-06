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


#define LENSFLARE_SIZE [128,128]

void
LensFlare_Draw(vector lightPos, float flareAlpha, vector lightColor, bool isBlinding)
{
	vector flarePos = project(lightPos) - (LENSFLARE_SIZE / 2);
	vector cameraPos = g_view.GetCameraOrigin();
	vector cameraAngle = g_view.GetCameraAngle();

	flareAlpha *= 0.5f;

	if (flareAlpha <= 0.0f) {
		return;
	}

	/* only when it's in our field of view and not occluded by the world */
	makevectors(cameraAngle);

	if ((normalize(lightPos - cameraPos) * v_forward) <= 0.3) {
		return;
	}

	traceline(cameraPos, lightPos, TRUE, self);

	if (trace_fraction != 1.0) {
		return;
	}

	float distAlpha = 1.0 - bound(0.0, vlen(cameraPos - lightPos) / 1024, 1.0);
	vector c = ((g_vidsize / 2) - project(lightPos)) * 1.25f;
	flareAlpha = bound(0.0, 1.0 - (vlen(c) / 350), 1.0f) * flareAlpha;
	flareAlpha *= distAlpha;
	distAlpha = 1.0 - bound(0.0, vlen(cameraPos - lightPos) / 512, 1.0);

	drawpic(flarePos, "textures/sfx/flare1", LENSFLARE_SIZE, lightColor * flareAlpha, 1.0f, DRAWFLAG_ADDITIVE);
	drawpic(flarePos - c * 0.1, "textures/sfx/flare2", LENSFLARE_SIZE, lightColor * flareAlpha, 1.0f, DRAWFLAG_ADDITIVE);
	drawpic(flarePos + c * 0.2, "textures/sfx/flare3", LENSFLARE_SIZE, lightColor * flareAlpha, 1.0f, DRAWFLAG_ADDITIVE);
	drawpic(flarePos - c * 0.3, "textures/sfx/flare4", LENSFLARE_SIZE, lightColor * flareAlpha, 1.0f, DRAWFLAG_ADDITIVE);
	drawpic(flarePos + c * 0.4, "textures/sfx/flare2", LENSFLARE_SIZE, lightColor * flareAlpha, 1.0f, DRAWFLAG_ADDITIVE);
	drawpic(flarePos - c * 0.5, "textures/sfx/flare3", LENSFLARE_SIZE, lightColor * flareAlpha, 1.0f, DRAWFLAG_ADDITIVE);

	if (isBlinding == false) {
		return;
	}

	flareAlpha = bound(0.0, 1.0 - (vlen(c) / 100), 1.0f) * flareAlpha;
	drawfill(video_mins, g_vidsize, lightColor, flareAlpha * distAlpha);
}
