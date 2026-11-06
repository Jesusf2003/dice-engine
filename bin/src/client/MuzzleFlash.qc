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

/* Client-side muzzle flash sprite used by ncRenderableEntity model events and
   the view model. The map entity env_muzzleflash is a game entity; this is the
   effect the SDK itself spawns. */

var bool autocvar_cg_muzzleDLight = true;
var vector autocvar_cg_muzzleDLightColor = [1,0.45,0];

class ncMuzzleFlash:ncEntity
{
public:
	void ncMuzzleFlash(void);

	virtual float predraw(void);

	int m_iAttachment;
	ncRenderableEntity m_eOwner;
	bool m_forceAdditive;
};

void
ncMuzzleFlash::ncMuzzleFlash(void)
{
	m_iAttachment = 0i;
	m_eOwner = __NULL__;
	scale = 1.0f;
	m_forceAdditive = false;
}

float
ncMuzzleFlash::predraw(void)
{
	vector muzzlePos;

	if (m_iAttachment) {
		muzzlePos = gettaginfo(m_eOwner, (float)m_iAttachment);
	} else {
		muzzlePos = origin;
	}

	if (autocvar_cg_muzzleDLight == true) {
		float p = dynamiclight_add(muzzlePos, 150, autocvar_cg_muzzleDLightColor);
		dynamiclight_set(p, LFIELD_STYLE, 64);
	}

	setorigin(this, muzzlePos);

	if (m_forceAdditive) {
		effects = EF_ADDITIVE;
	}

	angles = m_eOwner.angles;
	addentity(this);
	alpha -= frametime * 16.0f;

	if (m_eOwner.alpha == 0.0f) {
		alpha = 0.0f;
	}

	if (alpha <= 0.0) {
		Destroy();
	}

	return (PREDRAW_NEXT);
}

static ncMuzzleFlash
MuzzleFlash_Spawn(entity muzzleOwner, float muzzleScale, int muzzleModel)
{
	ncMuzzleFlash tempMuzzle = spawn(ncMuzzleFlash);

	tempMuzzle.m_eOwner = (ncRenderableEntity)muzzleOwner;
	tempMuzzle.scale = muzzleScale;
	tempMuzzle.alpha = 1.0f;
	tempMuzzle.modelindex = muzzleModel;
	setmodel(tempMuzzle, modelnameforindex(muzzleModel));
	setsize(tempMuzzle, [0,0,0], [0,0,0]);
	tempMuzzle.drawmask = MASK_GLOWS;
	return (tempMuzzle);
}

/** Additive muzzle flash that follows an attachment of muzzleOwner. */
void
EV_MuzzleFlash_CreateAdditive(entity muzzleOwner, int attachmentID, float muzzleScale, int muzzleModel)
{
	ncMuzzleFlash tempMuzzle = MuzzleFlash_Spawn(muzzleOwner, muzzleScale, muzzleModel);
	tempMuzzle.m_iAttachment = attachmentID;
	tempMuzzle.m_forceAdditive = true;
	setorigin(tempMuzzle, muzzleOwner.origin);
}

/** Muzzle flash at a fixed position, fading with muzzleOwner. */
void
EV_MuzzleFlash_CreateAtPos(entity muzzleOwner, vector pos, float muzzleScale, int muzzleModel)
{
	ncMuzzleFlash tempMuzzle = MuzzleFlash_Spawn(muzzleOwner, muzzleScale, muzzleModel);
	setorigin(tempMuzzle, pos);
}
