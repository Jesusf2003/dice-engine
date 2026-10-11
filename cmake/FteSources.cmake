#Platform-independent source lists, mirroring upstream's CMakeLists/Makefile groupings.
#Paths are absolute so the lists can be consumed from any subdirectory.

set(_E "${FTE_ROOT}/engine")

#GL-specific, but harmless when GL is disabled.
set(FTE_GL_FILES
	${_E}/gl/gl_backend.c
	${_E}/gl/gl_bloom.c
	${_E}/gl/gl_draw.c
	${_E}/gl/gl_rmain.c
	${_E}/gl/gl_rmisc.c
	${_E}/gl/gl_rsurf.c
	${_E}/gl/gl_screen.c
	${_E}/gl/gl_vidcommon.c
	${_E}/gl/glmod_doom.c
)

set(FTE_VK_FILES
	${_E}/vk/vk_backend.c
	${_E}/vk/vk_init.c
)

#The QC virtual machine and in-engine compiler.
set(FTE_QCVM_FILES
	${_E}/qclib/comprout.c
	${_E}/qclib/initlib.c
	${_E}/qclib/pr_edict.c
	${_E}/qclib/pr_exec.c
	${_E}/qclib/pr_multi.c
	${_E}/qclib/qcc_cmdlib.c
	${_E}/qclib/qcc_pr_comp.c
	${_E}/qclib/qcc_pr_lex.c
	${_E}/qclib/qccmain.c
	${_E}/qclib/qcd_main.c
	${_E}/qclib/qcdecomp.c
)

#Standalone QC compiler (shared by fteqcc and fteqccgui).
set(FTE_QCC_FILES
	${_E}/qclib/comprout.c
	${_E}/qclib/hash.c
	${_E}/qclib/qcc_cmdlib.c
	${_E}/qclib/qcc_pr_comp.c
	${_E}/qclib/qcc_pr_lex.c
	${_E}/qclib/qccmain.c
	${_E}/qclib/qcd_main.c
	${_E}/qclib/decomp.c
	${_E}/qclib/packager.c
)

#Common to both server-only and client+server builds.
set(FTE_COMMON_FILES
	${_E}/common/cmd.c
	${_E}/common/com_mesh.c
	${_E}/common/com_bih.c
	${_E}/common/common.c
	${_E}/common/json.c
	${_E}/common/crc.c
	${_E}/common/cvar.c
	${_E}/common/fs.c
	${_E}/common/fs_dzip.c
	${_E}/common/fs_pak.c
	${_E}/common/fs_stdio.c
	${_E}/common/fs_xz.c
	${_E}/common/fs_zip.c
	${_E}/common/gl_q2bsp.c
	${_E}/common/huff.c
	${_E}/common/log.c
	${_E}/common/mathlib.c
	${_E}/common/md4.c
	${_E}/common/md5.c
	${_E}/common/net_chan.c
	${_E}/common/net_ice.c
	${_E}/common/net_wins.c
	${_E}/common/plugin.c
	${_E}/common/pmove.c
	${_E}/common/pmovetst.c
	${_E}/common/pr_bgcmd.c
	${_E}/common/q1bsp.c
	${_E}/common/q2pmove.c
	${_E}/common/qvm.c
	${_E}/common/sha1.c
	${_E}/common/sha2.c
	${_E}/common/translate.c
	${_E}/common/zone.c

	#client files that the server needs too (hitmodels, skeletal objects, downloads...)
	${_E}/client/pr_skelobj.c
	${_E}/client/m_download.c
	${_E}/client/net_master.c
	${_E}/client/r_d3.c
	${_E}/gl/gl_heightmap.c
	${_E}/gl/gl_hlmdl.c
	${_E}/gl/gl_model.c

	${_E}/server/sv_move.c
	${_E}/server/sv_phys.c
	${_E}/server/world.c

	${FTE_QCVM_FILES}
	${_E}/qclib/hash.c

	${_E}/http/httpclient.c
)

#Headers, listed only so IDEs show them.
file(GLOB FTE_HEADER_FILES CONFIGURE_DEPENDS
	${_E}/common/*.h
	${_E}/client/*.h
	${_E}/server/*.h
	${_E}/gl/*.h
	${_E}/vk/*.h
	${_E}/qclib/*.h
	${_E}/http/*.h
)
list(FILTER FTE_HEADER_FILES EXCLUDE REGEX "/(r_bishaders|bymorphed|anorms)\\.h$")

set(FTE_SERVER_FILES
	${_E}/server/net_preparse.c
	${_E}/server/pr_cmds.c
	${_E}/server/pr_lua.c
	${_E}/server/pr_q1qvm.c
	${_E}/server/savegame.c
	${_E}/server/sv_ccmds.c
	${_E}/server/sv_chat.c
	${_E}/server/sv_cluster.c
	${_E}/server/sv_demo.c
	${_E}/server/sv_ents.c
	${_E}/server/sv_init.c
	${_E}/server/sv_main.c
	${_E}/server/sv_master.c
	${_E}/server/sv_mvd.c
	${_E}/server/sv_nchan.c
	${_E}/server/sv_rankin.c
	${_E}/server/sv_send.c
	${_E}/server/sv_sql.c
	${_E}/server/sv_user.c
	${_E}/server/svq2_ents.c
	${_E}/server/svq2_game.c
)

#Client-only files.
set(FTE_CLIENT_FILES
	${_E}/client/cl_cam.c
	${_E}/client/cl_demo.c
	${_E}/client/cl_ents.c
	${_E}/client/cl_ignore.c
	${_E}/client/cl_input.c
	${_E}/client/cl_main.c
	${_E}/client/cl_parse.c
	${_E}/client/cl_pred.c
	${_E}/client/cl_screen.c
	${_E}/client/cl_tent.c
	${_E}/client/clq2_cin.c
	${_E}/client/clq2_ents.c
	${_E}/client/console.c
	${_E}/client/fragstats.c
	${_E}/client/image.c
	${_E}/client/in_generic.c
	${_E}/client/keys.c
	${_E}/client/m_items.c
	${_E}/client/m_master.c
	${_E}/client/m_mp3.c
	${_E}/client/m_multi.c
	${_E}/client/m_options.c
	${_E}/client/m_script.c
	${_E}/client/m_native.c
	${_E}/client/m_single.c
	${_E}/client/menu.c
	${_E}/client/p_classic.c
	${_E}/client/p_null.c
	${_E}/client/p_script.c
	${_E}/client/pr_clcmd.c
	${_E}/client/pr_csqc.c
	${_E}/client/pr_menu.c
	${_E}/client/r_2d.c
	${_E}/client/r_part.c
	${_E}/client/r_partset.c
	${_E}/client/r_surf.c
	${_E}/client/renderer.c
	${_E}/client/renderque.c
	${_E}/client/roq_read.c
	${_E}/client/sbar.c
	${_E}/client/skin.c
	${_E}/client/snd_dma.c
	${_E}/client/snd_mem.c
	${_E}/client/snd_mix.c
	${_E}/client/snd_mp3.c
	${_E}/client/snd_ov.c
	${_E}/client/textedit.c
	${_E}/client/valid.c
	${_E}/client/view.c
	${_E}/client/wad.c
	${_E}/client/zqtp.c

	#generic renderer files (no longer gl-specific, for the most part)
	${_E}/gl/gl_alias.c
	${_E}/gl/gl_font.c
	${_E}/gl/gl_ngraph.c
	${_E}/gl/gl_rlight.c
	${_E}/gl/gl_shader.c
	${_E}/gl/gl_shadow.c
	${_E}/gl/gl_warp.c
	${_E}/gl/ltface.c

	${_E}/client/vid_headless.c
	${FTE_GL_FILES}
	${FTE_VK_FILES}
)

unset(_E)
