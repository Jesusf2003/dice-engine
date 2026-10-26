# tools

- `CMakeLists.txt` builds the C tools (imgtool, iqmtool, ...) into `bin/`.
- `assets/` Python scripts for preparing game assets.
- `debug/` Python helpers for debugging the engine and editors.

## assets/

Need Python 3 and Pillow: `python -m pip install -r tools/assets/requirements.txt`

| Script | What it does |
|---|---|
| `make_wad3.py <image> <texname> <out.wad> <out.png> <w> <h>` | WAD3 with one texture using its own 256 colour palette (looks much better than imgtool's Quake palette), plus a full colour PNG for `textures/` that FTE loads instead. `w`/`h` must be multiples of 16. |
| `split_sheet.py <sheet.png> <outdir> <prefix>` | Cuts a sprite sheet with a solid magenta background into one transparent PNG per sprite. Rows keep a shared height so animation frames stay aligned. |
| `make_marine_spr.py <pngdir> <out.spr> <feet_z>` | Builds FTE's 32-bit sprite (`.spr`, SPRITE32) from the cut Doom marine frames, with 8-direction angled frames. Frame layout is documented at the top of the script. Put a `!` in the output name (`marine!.spr`) to have FTE light it with the map lighting instead of fullbright. |

For what imgtool already covers (WAD2/WAD3 with the Quake palette, extracting WADs/BSPs, DDS/KTX) use `bin/imgtool64.exe`.

## debug/

| Script | What it does |
|---|---|
| `symaddr.py <exe> <hex offset>...` | Turns the crash offset from Windows' Application Error event into function + file:line using the exe's `.pdb`. Windows only. |
