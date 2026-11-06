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


bool
WorldText_Visible(vector p1, vector p2, vector ang)
{
	vector delta;
	float fov;

	if (distance2D(p1, p2) > 512) {
		return (false);
	}

	makevectors(ang);
	delta = normalize(p1 - p2);
	fov = delta * v_forward;

	if (fov > 0.3) {
		traceline(p2, p1, MOVE_WORLDONLY, self);
		if (trace_fraction == 1.0) {
			return (true);
		}
	}
	return (false);
}

void
WorldText_DrawString(vector org, string message)
{
	vector textPos = project(org) - [(stringwidth(message, FALSE, [12,12]) / 2), 0];
	Font_DrawText_RGB(textPos + [1,1], message, [0,0,0], FONT_CON);
	Font_DrawText(textPos, message, FONT_CON);
}

void
WorldText_DrawField(vector org, string message, float width)
{
	vector textPos = project(org) - [(width / 2), 0];
	Font_DrawField(textPos + [1,1], [width, g_hudres[1]], strcat("^x000", message), FONT_CON, AF_DEFAULT);
	Font_DrawField(textPos, [width, g_hudres[1]], message, FONT_CON, AF_DEFAULT);
}
