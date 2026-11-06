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
#   vmap     - builds the map compiler into tools/vmap/bin/vmap.exe (plus the DLLs it needs)
#   vmap-intellisense - only writes tools/vmap/compile_commands.json (VS Code include paths for vmap)
#   clean    - removes engine objects (keeps the third-party libs)
# Environment: GAME (default base), JOBS (default: nproc), NATIVE_PLUGINS.

set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
NUCLIDE="$ROOT/bin"
ENGINE="$ROOT/engine"
VMAP="$ROOT/tools/vmap"
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

# vmap needs glib, libxml2 and minizip. Installing them through pacman would force a full MSYS2
# upgrade (and a new GCC for the engine), so the packages are unpacked into tools/vmap/deps instead.
VMAP_PKGS="glib2-2.90.1-1 pcre2-10.49-1 libxml2-2.15.4-3 minizip-1.3.2-3"

vmap_deps() {
	local deps="$VMAP/deps"
	[ -f "$deps/ucrt64/lib/pkgconfig/minizip.pc" ] && return
	mkdir -p "$deps/pkgs"
	for p in $VMAP_PKGS; do
		local f="mingw-w64-ucrt-x86_64-$p-any.pkg.tar.zst"
		[ -f "$deps/pkgs/$f" ] || curl -sfL -o "$deps/pkgs/$f" "https://repo.msys2.org/mingw/ucrt64/$f"
		tar --zstd -xf "$deps/pkgs/$f" -C "$deps" ucrt64
	done
	# The .pc files say prefix=/ucrt64; point them at the unpacked copy.
	sed -i "s#^prefix=.*#prefix=$deps/ucrt64#" "$deps"/ucrt64/lib/pkgconfig/*.pc
	# vmap's bundled unzip.h includes "ioapi.h", which MSYS2 keeps in include/minizip/.
	sed -i 's#^Cflags: -I${includedir}$#Cflags: -I${includedir}/minizip -I${includedir}#' "$deps/ucrt64/lib/pkgconfig/minizip.pc"
}

# Copies the non-system DLLs an exe imports, recursively, from the vmap deps or the UCRT64 prefix.
copy_dlls() {
	local dll src
	for dll in $(objdump -p "$1" | sed -n 's/^\s*DLL Name: //p'); do
		[ -f "$2/$dll" ] && continue
		for src in "$VMAP/deps/ucrt64/bin/$dll" "/ucrt64/bin/$dll"; do
			if [ -f "$src" ]; then
				install -m 0755 "$src" "$2/"
				copy_dlls "$src" "$2"
				break
			fi
		done
	done
}

# Writes tools/vmap/compile_commands.json for VS Code IntelliSense (.vscode/c_cpp_properties.json),
# with the same flags as vmap's Makefiles: src/ and common/ are built from src/ with the pkg-config
# flags of glib, libxml2 and minizip; each libs/<lib>/ from its own folder.
vmap_compile_commands() {
	local cc pkgflags out="$VMAP/compile_commands.json" sep="" f dir flags
	cc="$(cygpath -m "$(command -v gcc)")"
	pkgflags="$(PKG_CONFIG_PATH="$VMAP/deps/ucrt64/lib/pkgconfig:/ucrt64/lib/pkgconfig" \
		pkg-config --cflags glib-2.0 libxml-2.0 minizip)"
	echo "[" > "$out"
	for f in "$VMAP"/src/*.c "$VMAP"/common/*.c "$VMAP"/libs/*/*.c; do
		case "$f" in
			"$VMAP"/libs/*) dir="$(dirname "$f")"; flags="-I../../include -I../../libs" ;;
			*) dir="$VMAP/src"; flags="$pkgflags -I../include -I../common -I../libs" ;;
		esac
		# only what the Makefiles build (src/ and common/ keep unbuilt q3map2 leftovers)
		grep -qw "$(basename "$f" .c)\.o" "$dir/Makefile" || continue
		printf '%s{"directory": "%s", "file": "%s", "command": "%s %s -c %s"}\n' "$sep" \
			"$(cygpath -m "$dir")" "$(cygpath -m "$f")" "$cc" "$flags" "$(cygpath -m "$f")" >> "$out"
		sep=","
	done
	echo "]" >> "$out"
	echo "vmap: tools/vmap/compile_commands.json"
}

do_vmap() {
	vmap_deps
	vmap_compile_commands
	# l_net uses Winsock on Windows, which the upstream Makefile does not link.
	# Windows' default 1 MB stack overflows in PatchMapDrawSurfs (Linux gives 8 MB), so ask for 16 MB.
	# The link rule only depends on the objects, so drop the old exe to make LDFLAGS changes take effect.
	rm -f "$VMAP/vmap.exe"
	PKG_CONFIG_PATH="$VMAP/deps/ucrt64/lib/pkgconfig:/ucrt64/lib/pkgconfig" \
		make -C "$VMAP" LDFLAGS="-lws2_32 -Wl,--stack,16777216" -j"$JOBS"
	mkdir -p "$VMAP/bin"
	install -m 0755 "$VMAP/vmap.exe" "$VMAP/bin/vmap.exe"
	copy_dlls "$VMAP/bin/vmap.exe" "$VMAP/bin"
	echo "vmap: tools/vmap/bin/vmap.exe"
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
		fteqcc|game|engine|fteqw|plugins|makelibs|vmap|clean) "do_$t" ;;
		vmap-intellisense) vmap_deps; vmap_compile_commands ;;
		*) echo "unknown target: $t" >&2; exit 1 ;;
	esac
done
