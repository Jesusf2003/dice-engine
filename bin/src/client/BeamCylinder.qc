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

/* Client side of the EV_BEAMCYLINDER event (a TE_BEAMCYLINDER-like expanding
   ring), sent by ncEntity's "Shockwave" input. The map entity env_shockwave
   is a game entity; this is only the effect the SDK itself can emit. */

#define BEAMCYLINDER_POLYCOUNT 32

class ncBeamCylinder:ncEntity
{
public:
	void ncBeamCylinder(void);

	virtual float predraw(void);

	float m_flRadius;
	string m_strTexture;
	float m_flStartFrame;
	float m_flFrameRate;
	float m_flLifeTime;
	float m_flHeight;
	float m_flNoiseAmp;
	vector m_vecColor;
	float m_flBrightness;
	float m_flScrollSpeed;
};

void
ncBeamCylinder::ncBeamCylinder(void)
{
}

float
ncBeamCylinder::predraw(void)
{
	vector startPos = origin;
	vector endPos = origin;
	float maxSize = (frame1time / m_flLifeTime); /* 0.0 - 1.0 */

	if (maxSize >= 1.0) {
		Destroy();
		return (PREDRAW_NEXT);
	}

	vector vecPlayer = g_view.GetCameraOrigin();
	vecPlayer[2] = origin[2] + (m_flHeight * 0.5f);

	makevectors(angles);
	startPos += v_forward * m_flRadius;
	endPos -= v_forward * m_flRadius;

	if (m_strTexture) {
		makevectors(angles);
		setproperty(VF_ORIGIN, vecPlayer);
		R_BeginPolygon(m_strTexture, DRAWFLAG_ADDITIVE, 0);

		for (float i = 0; i < BEAMCYLINDER_POLYCOUNT; i++) {
			vector point;
			vector jitter;
			float length = vlen(startPos - endPos) * 0.5 * maxSize;
			float theta = 2.0f * M_PI * (i / (BEAMCYLINDER_POLYCOUNT - 1));

			point = origin + [sin(theta), cos(theta)] * length;

			if (m_flNoiseAmp > 0) {
				/* nudge it a lil bit up/down left/right from its trajectory */
				jitter = v_right * (pseudorand((4 * time) + i + entnum) - 0.5);
				jitter += v_up * (pseudorand((4 * time) + i + 64.12 + entnum) - 0.5);
				jitter += v_right * (pseudorand(100 + (8 * time) + i + entnum) - 0.5);
				jitter += v_up * (pseudorand(100 + (8 * time) + i + 25.4 + entnum) - 0.5);
				point += jitter * m_flNoiseAmp;
			}

			R_PolygonVertex(point, [1, 0], m_vecColor / 255, m_flBrightness / 255);
		}

		R_EndPolygonRibbon(m_flHeight, [-1,0]);
	}

	frame1time += frametime;
	return (PREDRAW_NEXT);
}

/* basically re-implementing TE_BEAMCYLINDER */
void
EV_BeamCylinder_Parse(void)
{
	ncBeamCylinder tempCylinder = spawn(ncBeamCylinder);

	tempCylinder.origin[0] = readcoord();
	tempCylinder.origin[1] = readcoord();
	tempCylinder.origin[2] = readcoord();
	tempCylinder.angles[0] = readcoord();
	tempCylinder.m_flRadius = readcoord();
	tempCylinder.angles[2] = readcoord();
	tempCylinder.modelindex = readshort();
	tempCylinder.m_flStartFrame = readbyte();
	tempCylinder.model = modelnameforindex(tempCylinder.modelindex);
	tempCylinder.m_strTexture = spriteframe(tempCylinder.model, tempCylinder.m_flStartFrame, 0.0f);
	tempCylinder.m_flFrameRate = readbyte();
	tempCylinder.m_flLifeTime = readbyte() * 0.1f;
	tempCylinder.m_flHeight = readbyte();
	tempCylinder.m_flNoiseAmp = readbyte();
	tempCylinder.m_vecColor[0] = readbyte();
	tempCylinder.m_vecColor[1] = readbyte();
	tempCylinder.m_vecColor[2] = readbyte();
	tempCylinder.m_flBrightness = readbyte();
	tempCylinder.m_flScrollSpeed = readbyte();
	tempCylinder.frame1time = 0.0f;
	setorigin(tempCylinder, tempCylinder.origin);
	setsize(tempCylinder, [0,0,0], [0,0,0]);
	tempCylinder.drawmask = MASK_ENGINE;
}
