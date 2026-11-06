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

/*! \brief Client-Entity: Cubemap Reflection Marker */
/*!QUAKED env_cubemap (1 .5 0) (-8 -8 -8) (8 8 8)
# OVERVIEW
Specifies a location for which a cubemap will be generated when the
buildcubemaps console command is executed.

# KEYS
- "scale" : Texture dimension at which to render the cubemap. Default is "32".

# TRIVIA
This entity was introduced in Half-Life 2 (2004).

@ingroup cliententity
@ingroup pointentity
*/
class env_cubemap:ncEntity
{
public:
	void env_cubemap(void);
	virtual void SpawnKey(string,string);
	virtual bool CanSpawn(bool);

};

void
env_cubemap::SpawnKey(string strField, string strKey)
{
	switch (strField) {
	case "scale":
		cubemapSize = stof(strKey);
		if (cubemapSize <= 16) {
			cubemapSize = 16;
		} else if (cubemapSize <= 32) {
			cubemapSize = 32;
		} else if (cubemapSize <= 64) {
			cubemapSize = 64;
		} else if (cubemapSize <= 128) {
			cubemapSize = 128;
		} else {
			cubemapSize = 256;
		}
		break;
	default:
		super::SpawnKey(strField, strKey);
	}
}

bool
env_cubemap::CanSpawn(bool clientSide)
{
	return true;
}

void
env_cubemap::env_cubemap(void)
{
	cubemapSize = 32;
	isCSQC = true;
}

