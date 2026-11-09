# vmap Makefile 

# ws libs vmap uses
LIBOBJS=../libs/libddslib.a \
	../libs/libetclib.a \
	../libs/libfilematch.a \
	../libs/libl_net.a \
	../libs/libmathlib.a \
	../libs/libpicomodel.a

GLIB_CFLAGS=$(shell pkg-config --cflags glib-2.0)
GLIB_LDFLAGS=$(shell pkg-config --libs glib-2.0)

XML_CFLAGS=$(shell pkg-config --cflags libxml-2.0)
XML_LDFLAGS=$(shell pkg-config --libs libxml-2.0)

MINIZIP_CFLAGS=$(shell pkg-config --cflags minizip)
MINIZIP_LDFLAGS=$(shell pkg-config --libs minizip)

VMAP_CFLAGS=$(CFLAGS) $(GLIB_CFLAGS) $(XML_CFLAGS) $(MINIZIP_CFLAGS) -I. -I../include -I../common -I../libs
VMAP_LDFLAGS=$(LDFLAGS) -lm -lpthread -L../lib $(GLIB_LDFLAGS) $(XML_LDFLAGS) $(MINIZIP_LDFLAGS)

DO_CC=$(CC) $(VMAP_CFLAGS) -o $@ -c $<

.c.o:
	$(DO_CC)

VMAP_OBJS = \
	../common/cmdlib.o \
	../common/imagelib.o \
	../common/inout.o \
	../common/md4.o \
	../common/mutex.o \
	../common/polylib.o \
	../common/scriplib.o \
	../common/matlib.o \
	../common/threads.o \
	../common/vfs.o \
	bsp/brush.o \
	bsp/brush_primit.o \
	bsp/bsp.o \
	bsp_analyze.o \
	bsp_info.o \
	bsp_scale.o \
	bspfile_abstract.o \
	bspfile_ibsp.o \
	bspfile_rbsp.o \
	convert_ase.o \
	convert_bsp.o \
	convert_map.o \
	convert_obj.o \
	bsp/decals.o \
	exportents.o \
	bsp/facebsp.o \
	fixaas.o \
	bsp/fog.o \
	help.o \
	image.o \
	bsp/leakfile.o \
	light/light.o \
	light/light_bounce.o \
	light/light_trace.o \
	light/light_ydnar.o \
	light/lightmaps_ydnar.o \
	main.o \
	bsp/map.o \
	bsp/mesh.o \
	bsp/model.o \
	bsp/patch.o \
	path_init.o \
	bsp/portals.o \
	bsp/prtfile.o \
	shaders.o \
	bsp/surface.o \
	bsp/surface_extra.o \
	bsp/surface_foliage.o \
	bsp/surface_fur.o \
	bsp/surface_meta.o \
	bsp/tjunction.o \
	bsp/tree.o \
	vis/vis.o \
	vis/visflow.o \
	bsp/writebsp.o

# binary target
../vmap: $(VMAP_OBJS)
	$(CC) -o $@ $(VMAP_OBJS) $(LIBOBJS) $(VMAP_LDFLAGS)

clean:
	-rm -f ./../common/*.o
	-rm -f ./*.o ./bsp/*.o ./light/*.o ./vis/*.o
	-rm -f ../vmap

# object files
../common/cmdlib.o: ../common/cmdlib.c ../common/cmdlib.h
../common/imagelib.o: ../common/imagelib.c ../common/imagelib.h
../common/inout.o: ../common/inout.c ../common/inout.h
../common/md4.o: ../common/md4.c ../common/md4.h
../common/mutex.o: ../common/mutex.c ../common/mutex.h
../common/polylib.o: ../common/polylib.c ../common/polylib.h
../common/scriplib.o: ../common/scriplib.c ../common/scriplib.h
../common/matlib.o: ../common/matlib.c ../common/matlib.h
../common/threads.o: ../common/threads.c
../common/vfs.o: ../common/vfs.c ../common/vfs.h
bsp/brush.o: bsp/brush.c 
bsp/brush_primit.o: bsp/brush_primit.c
bsp/bsp.o: bsp/bsp.c
bsp_analyze.o: bsp_analyze.c
bsp_info.o: bsp_info.c
bsp_scale.o: bsp_scale.c
bspfile_abstract.o: bspfile_abstract.c
bspfile_ibsp.o: bspfile_ibsp.c
bspfile_rbsp.o: bspfile_rbsp.c
convert_ase.o: convert_ase.c
convert_bsp.o: convert_bsp.c
convert_map.o: convert_map.c
convert_obj.o: convert_obj.c
bsp/decals.o: bsp/decals.c
exportents.o: exportents.c
bsp/facebsp.o: bsp/facebsp.c
fixaas.o: fixaas.c
bsp/fog.o: bsp/fog.c
help.o: help.c
image.o: image.c
bsp/leakfile.o: bsp/leakfile.c
light/light.o: light/light.c
light/light_bounce.o: light/light_bounce.c
light/light_trace.o: light/light_trace.c
light/light_ydnar.o: light/light_ydnar.c
light/lightmaps_ydnar.o: light/lightmaps_ydnar.c
main.o: main.c
bsp/map.o: bsp/map.c
bsp/mesh.o: bsp/mesh.c
bsp/model.o: bsp/model.c
bsp/patch.o: bsp/patch.c
path_init.o: path_init.c
bsp/portals.o: bsp/portals.c
bsp/prtfile.o: bsp/prtfile.c
shaders.o: shaders.c
bsp/surface.o: bsp/surface.c
bsp/surface_extra.o: bsp/surface_extra.c
bsp/surface_foliage.o: bsp/surface_foliage.c
bsp/surface_fur.o: bsp/surface_fur.c
bsp/surface_meta.o: bsp/surface_meta.c
bsp/tjunction.o: bsp/tjunction.c
bsp/tree.o: bsp/tree.c
vis/vis.o: vis/vis.c
vis/visflow.o: vis/visflow.c
bsp/writebsp.o: bsp/writebsp.c
