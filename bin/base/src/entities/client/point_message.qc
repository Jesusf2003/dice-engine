/*
 * Copyright (c) 2016-2022 Vera Visions LLC.
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

/*! \brief Client-Entity: In-world Debug Text */
/*!QUAKED point_message (0.2 1 0.2) (-8 -8 -8) (8 8 8)
# OVERVIEW
Client-side overlay/message that is projected in relation to its position
in 3D space.

# KEYS
- "message" : The message to display.
- "radius" : The radius in which it will appear.

# NOTES
Used for zoo and test maps in which less interactive overlays are desired.

# TRIVIA
This entity was introduced in Half-Life 2 (2004).

@ingroup cliententity
@ingroup pointentity
*/
class point_message:ncEntity
{
public:
	void point_message(void);
	virtual void SpawnKey(string, string);
	virtual bool CanSpawn(bool);
	virtual void postdraw(void);

private:
	float m_flRadius;
	string m_strMessage;
};

void
point_message::SpawnKey(string strField, string strKey)
{
	switch (strField) {
		case "radius":
			m_flRadius = stof(strKey);
			break;
		case "message":
			m_strMessage = strKey;
			break;
		case "origin":
			origin = stov(strKey);
			setorigin(this, origin);
			break;
		default:
			super::SpawnKey(strField, strKey);
	}
}

void
point_message::point_message(void)
{
	m_flRadius = 512;
	m_strMessage = "No message";
	isCSQC = true;
}

bool
point_message::CanSpawn(bool clientSide)
{
	return true;
}

void
point_message::postdraw(void)
{
	vector cameraPos = g_view.GetCameraOrigin();

	if (vlen(origin - cameraPos) > m_flRadius) {
		return;
	}

	if (WorldText_Visible(origin, cameraPos, g_view.GetCameraAngle()) == false) {
		return;
	}

	vector textPos = project(origin) - [(stringwidth(m_strMessage, FALSE, [12,12]) / 2), 0];
	Font_DrawText(textPos, m_strMessage, FONT_CON);
}
