#!/bin/bash
# Windows (MSYS2 UCRT64) build wrapper for this workspace: FTEQW at the root, Nuclide in ./bin.
# Runs the steps from bin/Documentation/Building.md with the parameters MinGW-w64 needs.
#
# Usage, from an MSYS2 UCRT64 terminal in this directory:
#   ./build_win64.sh [targets...]          e.g.  GAME=base ./build_win64.sh game engine
# or from VS Code: Terminal > Run Build Task (.vscode/tasks.json).
# Targets (default: fteqcc game engine plugins):
#   fteqcc   - builds the QuakeC compiler and installs it as bin/fteqcc.exe   (make fteqcc)
#   game     - builds progs.dat/csprogs.dat/menu.dat for $GAME               (make game GAME=...)
#   engine   - builds the branded engine from bin/$GAME/engine.h              (make engine-win64 GAME=...)
#   fteqw    - builds the generic, unbranded engine as bin/fteqw.exe          (make fteqw)
#   plugins  - builds the plugins listed in bin/$GAME/PLUGINS or $NATIVE_PLUGINS
#   makelibs - builds the static third-party libs (zlib, png, jpeg, ogg, vorbis, opus, speex, freetype)
#   clean    - removes engine objects (keeps the third-party libs)
# Environment: GAME (default base), JOBS (default: nproc), NATIVE_PLUGINS.

set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
NUCLIDE="$ROOT/bin"
ENGINE="$ROOT/engine"
GAME="${GAME:-base}"
JOBS="${JOBS:-$(nproc)}"

if [ "$MSYSTEM" != "UCRT64" ] && [ "$MSYSTEM" != "MINGW64" ]; then
	echo "error: run this from an MSYS2 UCRT64 shell (MSYSTEM=$MSYSTEM)" >&2
	exit 1
fi

# Nuclide expects FTEQW in bin/ThirdParty/fteqw. Junctions point there instead of cloning a second copy.
ensure_thirdparty() {
	local tp="$NUCLIDE/ThirdParty/fteqw"
	# MSYS2's own /usr/bin/cmd is a wrapper script, not Windows' cmd.exe.
	local wincmd
	wincmd="$(cygpath -u "${COMSPEC:-C:\\Windows\\System32\\cmd.exe}")"
	mkdir -p "$tp"
	# Junctions store absolute targets, so after moving the workspace they still point at the old copy.
	for d in engine plugins; do
		if [ "$(realpath -m "$tp/$d")" != "$(realpath "$ROOT/$d")" ]; then
			if [ -e "$tp/$d" ] || [ -L "$tp/$d" ]; then
				"$wincmd" //c "rmdir $(cygpath -w "$tp/$d")" > /dev/null	# removes only the junction, not its target
			fi
			"$wincmd" //c "mklink /J $(cygpath -w "$tp/$d") $(cygpath -w "$ROOT/$d")" > /dev/null
		fi
	done
}

# The engine Makefile shares one object dir between FTE_CONFIGs and does not notice config changes,
# so stale objects (e.g. plugin.o referencing Plug_Q3_Init) break the link. Wipe them on a switch.
select_config() {
	local stamp="$ENGINE/release/.fte_config"
	mkdir -p "$ENGINE/release"
	if [ "$(cat "$stamp" 2>/dev/null)" != "$1" ]; then
		rm -rf "$ENGINE/release/m_mgw64"
		echo "$1" > "$stamp"
	fi
}

do_makelibs() {
	# Serial on purpose: the libpng.a and libpng.pc rules race on the same configure under -j.
	make -C "$ENGINE" makelibs FTE_TARGET=win64
}

do_fteqcc() {
	make -C "$ENGINE" qcc-rel FTE_TARGET=win64 -j"$JOBS"
	install -m 0755 "$ENGINE/release/fteqcc64.exe" "$NUCLIDE/fteqcc.exe"
}

do_game() {
	[ -f "$NUCLIDE/fteqcc.exe" ] || do_fteqcc
	make -C "$NUCLIDE" game GAME="$GAME"
}

do_engine() {
	do_makelibs
	select_config "$GAME"
	# Nuclide's engine-win64 target is the exe itself with no prerequisites; remove it so engine edits get rebuilt.
	rm -f "$ENGINE/release/${GAME}64.exe"
	make -C "$NUCLIDE" engine-win64 GAME="$GAME" NATIVE_PLUGINS="${NATIVE_PLUGINS:-}" -j"$JOBS"
	local name
	name="$(cd "$NUCLIDE" && ./Tools/get-name.sh "$GAME")"
	install -m 0755 "$NUCLIDE/build/$name-win/${name}_x64.exe" "$NUCLIDE/${name}_x64.exe"
	echo "Engine: bin/${name}_x64.exe  (run: ${name}_x64.exe +game $GAME)"
}

do_fteqw() {
	do_makelibs
	select_config fteqw
	make -C "$ENGINE" m-rel FTE_TARGET=win64 -j"$JOBS"
	install -m 0755 "$ENGINE/release/fteqw64.exe" "$NUCLIDE/fteqw.exe"
	echo "Engine: bin/fteqw.exe  (run: fteqw.exe +game $GAME)"
}

do_plugins() {
	local plugins="${NATIVE_PLUGINS:-$(head -n 1 "$NUCLIDE/$GAME/PLUGINS" 2>/dev/null | tr -d '\r' || true)}"
	if [ -z "$plugins" ]; then
		echo "No plugins requested (set NATIVE_PLUGINS or fill bin/$GAME/PLUGINS)."
		return
	fi
	make -C "$ENGINE" plugins-rel NATIVE_PLUGINS="$plugins" FTE_TARGET=win64 FTE_CONFIG="$GAME"
	find "$ENGINE/release/" -maxdepth 1 -name "fteplug_*.dll" -exec install -m 0755 '{}' "$NUCLIDE/" \;
}

do_clean() {
	make -C "$ENGINE" clean FTE_TARGET=win64
	rm -f "$ENGINE/release/.fte_config"
}

ensure_thirdparty
[ $# -eq 0 ] && set -- fteqcc game engine plugins
for t in "$@"; do
	echo "=== $t (GAME=$GAME) ==="
	case "$t" in
		fteqcc|game|engine|fteqw|plugins|makelibs|clean) "do_$t" ;;
		*) echo "unknown target: $t" >&2; exit 1 ;;
	esac
done
