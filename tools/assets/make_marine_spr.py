"""Builds an FTE SPRITE32 (.spr, 32-bit RGBA) from the cut Doom marine PNGs.

usage: make_marine_spr.py <pngdir> <out.spr> <feet_z>
feet_z: height of the entity's feet relative to its origin (the player's mins_z).

Frames (sprite frame index = .frame in QC):
  0-3  walk A-D   angled (8 directions)
  4    attack E   angled
  5    fire F     angled
  6    pain G     angled
  7-13 death H-N  single
  14-22 gib O-W   single
Angled groups follow FTE's renderer: index 0 = front, 2 = its left side, 4 = back, 6 = its right side.
Doom only ships rotations 1-5, so 6-8 are rotations 4-2 mirrored.
"""
import math
import os
import struct
import sys

from PIL import Image, ImageOps

pngdir, out, feet_z = sys.argv[1], sys.argv[2], float(sys.argv[3])

SPR_VP_PARALLEL_UPRIGHT = 0
SPR_SINGLE, SPR_ANGLED = 0, 2


def load(name, mirror=False):
	img = Image.open(os.path.join(pngdir, name + ".png")).convert("RGBA")
	if mirror:
		img = ImageOps.mirror(img)
	return img


def frame_bytes(img):
	"""dspriteframe_t + RGBA rows, pivot at the middle of the feet."""
	w, h = img.size
	alpha = img.getchannel("A").load()
	rows = [y for y in range(h) if any(alpha[x, y] for x in range(w))]
	feet = rows[-1]
	# horizontal pivot: median of the opaque pixels near the feet, so the gun sticking out doesn't shift the body
	band = range(max(rows[0], feet - max(4, (feet - rows[0]) // 4)), feet + 1)
	xs = sorted(x for y in band for x in range(w) if alpha[x, y])
	pivot = xs[len(xs) // 2]
	origin_x = -pivot
	origin_y = int(round(feet_z)) + feet + 1	# top edge, so the feet land on feet_z
	return struct.pack("<4i", origin_x, origin_y, w, h) + img.tobytes()


def angled(names):
	"""names: Doom rotations 1-5."""
	imgs = [load(n) for n in names] + [load(n, True) for n in (names[3], names[2], names[1])]
	data = struct.pack("<i", SPR_ANGLED) + struct.pack("<i", len(imgs)) + struct.pack("<8f", *([0.1] * 8))
	return data + b"".join(frame_bytes(i) for i in imgs), imgs


def single(name):
	img = load(name)
	return struct.pack("<i", SPR_SINGLE) + frame_bytes(img), [img]


frames = []
for col in range(1, 7):	# walk A-D, attack E, fire F
	frames.append(angled([f"marine_r{r}_{col:02d}" for r in range(1, 6)]))
frames.append(angled([f"marine_r6_{c:02d}" for c in range(1, 6)]))	# pain G
frames += [single(f"marine_death_{i:02d}") for i in range(1, 8)]
frames += [single(f"marine_gib_{i:02d}") for i in range(1, 10)]

allimgs = [i for _, imgs in frames for i in imgs]
maxw = max(i.width for i in allimgs)
maxh = max(i.height for i in allimgs)
radius = math.sqrt((maxw / 2) ** 2 + maxh ** 2)

header = struct.pack("<4siifiiifi", b"IDSP", 32, SPR_VP_PARALLEL_UPRIGHT, radius, maxw, maxh, len(frames), 0.0, 0)
with open(out, "wb") as f:
	f.write(header + b"".join(d for d, _ in frames))
print(f"{out}: {len(frames)} frames, max {maxw}x{maxh}, {os.path.getsize(out)} bytes")
