"""Cuts a sprite sheet with a solid key colour background into one transparent PNG per sprite.

usage: split_sheet.py <sheet.png> <outdir> <prefix>
Rows are found by empty horizontal lines, sprites inside a row by empty columns.
Each sprite keeps the full height of its row so frames of one animation stay aligned.
"""
import os
import sys

from PIL import Image

sheet, outdir, prefix = sys.argv[1:4]
KEY = (255, 0, 255)
MIN_ROW_HEIGHT = 12	# thinner bands are the separator line and the credits text
ROW_NAMES = {7: "death", 8: "gib"}

im = Image.open(sheet).convert("RGB")
w, h = im.size
px = im.load()

def runs(flags):
	out, start = [], None
	for i, f in enumerate(flags + [False]):
		if f and start is None:
			start = i
		elif not f and start is not None:
			out.append((start, i - 1))
			start = None
	return out

rows = [r for r in runs([any(px[x, y] != KEY for x in range(w)) for y in range(h)]) if r[1] - r[0] + 1 >= MIN_ROW_HEIGHT]

os.makedirs(outdir, exist_ok=True)
count = 0
for rowidx, (y0, y1) in enumerate(rows, 1):
	cols = runs([any(px[x, y] != KEY for y in range(y0, y1 + 1)) for x in range(w)])
	rowname = ROW_NAMES.get(rowidx, f"r{rowidx}")
	for colidx, (x0, x1) in enumerate(cols, 1):
		sprite = im.crop((x0, y0, x1 + 1, y1 + 1)).convert("RGBA")
		sprite.putdata([(r, g, b, 0) if (r, g, b) == KEY else (r, g, b, 255) for r, g, b, _ in sprite.getdata()])
		name = f"{prefix}_{rowname}_{colidx:02d}.png"
		sprite.save(os.path.join(outdir, name))
		count += 1
	print(f"row {rowidx} ({rowname}): {len(cols)} sprites")
print(f"{count} sprites -> {outdir}")
