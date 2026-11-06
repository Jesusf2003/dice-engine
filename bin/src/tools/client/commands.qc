/*
 * Copyright (c) 2025 Vera Visions LLC
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

/* Developer console commands, client side. Several only forward to the server
   (`sv ...`, src/tools/server/commands.qc) or to the client command parser on
   the server (`cmd ...`). */

float(vector org, float radius, vector lightcolours, optional float style, optional string cubemapname, optional float pflags) dynamiclight_spawnstatic2 = #0:dynamiclight_spawnstatic;

static void
CMD_testLight(void)
{
	float p;
	string cubeTexture = argv(1);

	if (STRING_SET(cubeTexture)) {
		p = dynamiclight_spawnstatic2(g_view.GetCameraOrigin(), 512, [1,1,1], 0, cubeTexture);
		dynamiclight_set(p, LFIELD_ANGLES, g_view.GetCameraAngle());
		dynamiclight_set(p, LFIELD_FLAGS, LFLAG_NORMALMODE | LFLAG_REALTIMEMODE | LFLAG_SHADOWMAP);
		dynamiclight_set(p, LFIELD_STYLE, 64);
	} else {
		p = dynamiclight_spawnstatic2(g_view.GetCameraOrigin(), 512, [1,1,1], 0);
		dynamiclight_set(p, LFIELD_STYLE, 2);
	}


}

static void
CMD_testPointLight(void)
{
	makevectors(g_view.GetCameraAngle());
	traceline(g_view.GetCameraOrigin(), g_view.GetCameraOrigin() + (v_forward * 4096.0f), MOVE_NORMAL, pSeat->m_ePlayer);
	dynamiclight_spawnstatic2(trace_endpos + (v_forward * -16.0f), 1024, [1,1,1]);
}

static void
CMD_dev_modeltest(void)
{
	string modelName = argv(1);

	if (STRING_SET(modelName)) {
		entity mt = spawn();
		mt.drawmask = MASK_ENGINE;
		setmodel(mt, argv(1));
		setsize(mt, [0,0,0], [0,0,0]);
		setorigin(mt, g_view.GetCameraOrigin());
		mt.angles = g_view.GetCameraAngle();
		mt.angles[0] = mt.angles[2] = 0;
	}
}

static void
CMD_dev_sunpos(void)
{
	vector sunpos, sunang;
	vector lepos, leang;

	makevectors(g_view.GetCameraAngle());
	sunpos = v_forward * -1;
	sunang = vectoangles(sunpos);

	makevectors(g_view.GetCameraAngle());
	lepos = v_forward * -1;
	leang = vectoangles(lepos);
	leang[1] -= 180;
	leang[0] *= -1;

	localcmd(sprintf("r_shadows_throwdirection %v\n", sunpos));
	print(sprintf("env_sun: pitch: %d; angle: %d\n", -sunang[0], sunang[1]));
	print(sprintf("light_environment: sunangle: %d; pitch: %d\n", leang[1], leang[0]));
}

static void
CMD_dev_measure(void)
{
	static vector measurepos;

	if (lengthSquared(measurepos) > 0.0f) {
		measurepos = g_view.GetCameraOrigin();
		CSQC_Parse_CenterPrint(sprintf( "First marker set at\n%v", measurepos));
	} else {
		CSQC_Parse_CenterPrint(sprintf("Distance: %d\n", vlen(measurepos - g_view.GetCameraOrigin())));
		measurepos = [0,0,0];
	}
}

static void
CMD_getpos(void)
{
	printf("setpos %v;setang -%v\n", g_view.GetCameraOrigin(), g_view.GetCameraAngle());
}

static void
CMD_setang(void)
{
    g_view.SetCameraAngle(stov(argv(1)));
    g_view.SetClientAngle(stov(argv(1)));
}

static void
CMD_titles_test(void)
{
	GameMessage_Setup(argv(1), 0);
}

static void
CMD_way_menu(void)
{
	Way_Autoload();
	Textmenu_Call("WAY_MENU");
}

static void
CMD_view_geomtest(void)
{
	printf("geomset %s %s\n", argv(1), argv(2));
	//Weapons_SetGeomset(sprintf("geomset %s %s\n", argv(1), argv(2)));
}

static void
CMD_player_geomtest(void)
{
	setcustomskin(pSeat->m_ePlayer, "", sprintf("geomset %s %s\n", argv(1), argv(2)));
}

/* Wishlist: list size and potential vmem usage */
static void
CMD_ListModels(void)
{
	string tmp;
	int i = 0i;
	int actual_count = 0i;

	for (i = -4096; i < 4096; i++) {
		tmp = modelnameforindex(i);

		if (strlen(tmp) > 1) {
			printf("%i %s\n", i, tmp);
			actual_count++;
		}
	}

	printf("\t%i total models loaded\n", actual_count);
}

/* Wishlist: list size and samplerate/bitdepth */
static void
CMD_ListSounds(void)
{
	string tmp;
	int i = 0i;
	int actual_count = 0i;
	float length;
	float total_length = 0.0f;

	for (i = 0; i < 4096; i++) {
		tmp = soundnameforindex(i);
		length = soundlength(tmp);

		if (strlen(tmp) > 1) {
			printf("%i %s (%f seconds)\n", i, tmp, length);
			actual_count++;
			total_length += length;
		}
	}
	printf("\t%i total samples loaded\n", actual_count);
	printf("\t%f seconds worth of samples\n", total_length);
}

static void
CMD_ListParticles(void)
{
	string tmp;
	int i = 0i;
	int actual_count = 0i;

	for (i = -4096; i < 4096; i++) {
		tmp = particleeffectquery(i, 0);

		if (strlen(tmp) > 1) {
			printf("%i %s\n", i, tmp);
			actual_count++;
		}
	}
	printf("\t%i total particles loaded\n", actual_count);
}

static void
CMD_ListTitles(void)
{
	int i = 0i;

	for (i = 0; i < g_titles_count; i++) {
		printf("%s, (x: %d, y: %d)\n", 
			g_titles[i].m_strName, g_titles[i].m_flPosX, g_titles[i].m_flPosY);
		printf("\teffect: %i, fade-in: %f, fade-out: %f\n", 
			g_titles[i].m_iEffect, g_titles[i].m_flFadeIn, g_titles[i].m_flFadeOut);
		printf("\thold-time: %f, fx-time: %f\n", 
			g_titles[i].m_flHoldTime, g_titles[i].m_flFXTime);
		printf("\tcolor 1: %v, color 2: %v\n", 
			g_titles[i].m_vecColor1, g_titles[i].m_vecColor2);
	}
	printf("\t%i total titles loaded\n", g_titles_count);
}

static void
CMD_ListModelFramegroups(void)
{
	string modelName = argv(1);
	float modelIndex = getmodelindex(modelName);
	string temp = "";

	printf("// model: %S\n", modelName);

	print("typedef enum {\n");
	for (int i = 0; i < modelframecount(modelIndex); i++) {
		temp = frametoname(modelIndex, i);
		printf("\t%s = %i,\n", strtoupper(temp), i);
	}
	print("} framegroups_e;\n");
}

void
Tools_Init(void)
{
	registercommand("showModelSpawner");
	registercommand("showEditor");
	registercommand("showSunEditor");
	registercommand("killClass");
	registercommand("killMovables");
	registercommand("listAmmo");
	registercommand("listClientEntityDef");
	registercommand("listClientSoundDef");
	registercommand("listEntityData");
	registercommand("listEntityDef");
	registercommand("listEntitiesByClass");
	registercommand("listEntitySpawnData");
	registercommand("listFramegroups");
	registercommand("listInventory");
	registercommand("listMapTweaks");
	registercommand("listModels");
	registercommand("listParticles");
	registercommand("listServerEntityDef");
	registercommand("listServerSoundDef");
	registercommand("listEntitiesByTarget");
	registercommand("listSounds");
	registercommand("listTargets");
	registercommand("listTitles");
	registercommand("ragdollCreate");
	registercommand("traceMaterial");
	registercommand("input");
	registercommand("respawnEntities");
	registercommand("spawnDef");
	registercommand("teleport");
	registercommand("teleportToClass");
	registercommand("trigger");
	registercommand("nodeAdd");
	registercommand("nodeDel");
	registercommand("nodeFlags");
	registercommand("nodeLink");
	registercommand("nodeRadius");
	registercommand("nodeOffset");
	registercommand("nodeUnlink");
	registercommand("clearDecals");
	registercommand("testLight");
	registercommand("testPointLight");
	registercommand("getpos");
	registercommand("setpos");
	registercommand("setang");
	registercommand("titles_test");
	registercommand("buildcubemaps");
	registercommand("dev_sunpos");
	registercommand("dev_measure");
	registercommand("view_geomtest");
	registercommand("player_geomtest");
	registercommand("way_menu");
	registercommand("dev_modeltest");
	registercommand("giveInventoryItem");
	registercommand("removeInventoryItem");
	registercommand("removeAllInventoryItems");
	registercommand("reloadClientLocale");
	registercommand("listUserInfo");

	/* text menus of the waypoint editor */
	Way_Init();
}

bool
Tools_ConsoleCommand(string sCMD)
{
	switch (argv(0)) {
	case "listFramegroups":
		CMD_ListModelFramegroups();
		break;
	case "listTitles":
		CMD_ListTitles();
		break;
	case "listModels":
		CMD_ListModels();
		break;
	case "listSounds":
		CMD_ListSounds();
		break;
	case "listParticles":
		CMD_ListParticles();
		break;
	case "listClientSoundDef":
		SoundDef_DebugList();
		break;
	case "listServerSoundDef":
		localcmd("sv listSoundDef\n");
		break;
	case "clearDecals":
		CMD_Cleardecals();
		break;
	case "testLight":
		CMD_testLight();
		break;
	case "testPointLight":
		CMD_testPointLight();
		break;
	case "dev_modeltest":
		CMD_dev_modeltest();
		break;
	case "dev_sunpos":
		CMD_dev_sunpos();
		break;
	case "dev_measure":
		CMD_dev_measure();
		break;
	case "getpos":
		CMD_getpos();
		break;
	case "setang":
		CMD_setang();
		break;
	case "buildcubemaps":
		Cubemaps_Build();
		break;
	case "titles_test":
		CMD_titles_test();
		break;
	case "way_menu":
		CMD_way_menu();
		break;
	case "view_geomtest":
		CMD_view_geomtest();
		break;
	case "player_geomtest":
		CMD_player_geomtest();
		break;
	case "listClientEntityDef":
		EntityDef_DebugList();
		break;
	case "ragdollCreate":
		ncRagdoll_Create(argv(1));
		break;
	case "showModelSpawner":
		VGUI_ModelSpawner();
		break;
	case "reloadClientLocale":
		Locale_Reload();
		break;
	case "showEditor":
		VGUI_ShowEditor();
		break;
	case "showSunEditor":
		VGUI_ShowSunEditor();
		break;
	case "listUserInfo":
		g_userInfo.DebugPrint();
		break;
	case "killClass":
	case "killMovables":
	case "trigger":
	case "input":
	case "listMapTweaks":
	case "listTargets":
	case "teleport":
	case "teleportToClass":
	case "respawnEntities":
	case "spawnDef":
	case "traceMaterial":
	case "listEntitySpawnData":
	case "listEntitiesByClass":
	case "listEntityData":
	case "listEntitiesByTarget":
		localcmd(sprintf("sv %s\n", sCMD));
		break;
	case "setpos":
	case "listInventory":
	case "giveInventoryItem":
	case "removeInventoryItem":
	case "removeAllInventoryItems":
	case "listAmmo":
		localcmd(sprintf("cmd %s\n", sCMD));
		break;
	/* navmesh shortcuts for `sv way`; they act on the node closest to you.
	   nodeLink, nodeUnlink and nodeFlags take two steps: run them at one node, then at the other. */
	case "nodeAdd":
		localcmd("sv way add\n");
		break;
	case "nodeDel":
		localcmd("sv way delete\n");
		break;
	case "nodeLink":
		localcmd("sv way connect2\n");
		break;
	case "nodeUnlink":
		localcmd("sv way unlink2\n");
		break;
	case "nodeFlags":
		localcmd(sprintf("sv way flag %s\n", argv(1)));
		break;
	case "nodeRadius":
		localcmd(sprintf("sv way radius %s\n", argv(1)));
		break;
	case "nodeOffset":
		localcmd(sprintf("sv way move %s %s %s\n", argv(1), argv(2), argv(3)));
		break;
	case "listEntityDef":
	case "listServerEntityDef":
		localcmd("sv listEntityDef\n");
		break;
	default:
		return (false);
	}

	return (true);
}
