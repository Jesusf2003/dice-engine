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

void Commandmenu_Open(void);
void Commandmenu_Close(void);

static void
CMD_vote(void)
{
	if (argv(1) == "yes") {
		sendevent("VoteY", "");
	} else if (argv(1) == "no") {
		sendevent("VoteN", "");
	}
}

void
CMD_Cleardecals(void)
{
	for (entity e = world; (e = find(e, ::classname, "tempdecal"));) {
		ncDecal dcl = (ncDecal)e;
		dcl.m_strShader = "";
	}
}

void
ncWeapon_SelectWeapon(ncWeapon nextWeapon)
{
	if (!nextWeapon) {
		pSeat->m_iHUDWeaponSelected = 0i;
		return;
	}

	/* this gets passed in ncPlayer::ClientInputFrame() to the server */
	pSeat->m_iHUDWeaponSelected = nextWeapon.GetSharedID();
	pSeat->m_flHUDWeaponSelectTime = time + 0.5f;
}

bool
Cmd_Parse(string sCMD)
{
	ncClient localPlayer = (ncClient)pSeat->m_ePlayer;
	switch (argv(0)) {

	case "slot1":
		Textmenu_Input(0);
		break;
	case "slot2":
		Textmenu_Input(1);
		break;
	case "slot3":
		Textmenu_Input(2);
		break;
	case "slot4":
		Textmenu_Input(3);
		break;
	case "slot5":
		Textmenu_Input(4);
		break;
	case "slot6":
		Textmenu_Input(5);
		break;
	case "slot7":
		Textmenu_Input(6);
		break;
	case "slot8":
		Textmenu_Input(7);
		break;
	case "slot9":
		Textmenu_Input(8);
		break;
	case "slot10":
		Textmenu_Input(9);
		break;

	case "vote":
		CMD_vote();
		break;
	case "callvote":
		sendevent("CallVote", "s", substring(sCMD, 9, strlen(sCMD)-9));
		break;
	case "+zoomin":
		pSeat->m_iZoomed = true;
		break;
	case "-zoomin":
		pSeat->m_iZoomed = false;
		break;
	case "+attack2":
		pSeat->m_iInputAttack2 = true;
		break;
	case "-attack2":
		pSeat->m_iInputAttack2 = false;
		break;
	case "+reload":
		pSeat->m_iInputReload = true;
		break;
	case "-reload":
		pSeat->m_iInputReload = false;
		break;
	case "+activate":
	case "+use":
		pSeat->m_iInputUse = true;
		break;
	case "-activate":
	case "-use":
		pSeat->m_iInputUse = false;
		break;
	case "+duck":
		if (autocvar_pm_crouchToggle == true)
			pSeat->m_iInputDuck = 1 - pSeat->m_iInputDuck;
		else
			pSeat->m_iInputDuck = TRUE;

		pSeat->m_bSpecInput = true;
		break;
	case "-duck":
		if (autocvar_pm_crouchToggle == false)
			pSeat->m_iInputDuck = FALSE;

		pSeat->m_bSpecInput = false;
		break;
	case "+prone":
		pSeat->m_iInputProne = true;
		break;
	case "-prone":
		pSeat->m_iInputProne = false;
		break;
	case "+sprint":
		pSeat->m_iSprinting = true;
		break;
	case "-sprint":
		pSeat->m_iSprinting = false;
		break;
	case "goprone":
		pSeat->m_dForceStance = STANCE_PRONE;
		break;
	case "gocrouch":
		pSeat->m_dForceStance = STANCE_CROUCH;
		break;
	case "+gostand":
		if (pSeat->m_dForceStance != STANCE_DEFAULT) {
			pSeat->m_dForceStance = STANCE_DEFAULT;
		} else {
			pSeat->m_iInputJump = true;
		}
		break;
	case "-gostand":
		pSeat->m_iInputJump = false;
		break;
	case "weapnext":
		ncWeapon_SelectWeapon(localPlayer.GetNextWeapon());
		break;
	case "weapprev":
		ncWeapon_SelectWeapon(localPlayer.GetPreviousWeapon());
		break;
	case "weaplast":
		ncWeapon_SelectWeapon(localPlayer.GetLastWeapon());
		break;
	case "+showscores":
		pSeat->m_iScoresVisible = TRUE;
		break;
	case "-showscores":
		pSeat->m_iScoresVisible = FALSE;
		break;
	case "addChatLine":
		CSQC_Parse_Print(argv(1), PRINT_CHAT);
		break;
	case "motd":
		print(MOTD_GetTextBody());
		break;

	/* XR binds, engine binds them currently */
	case "+attack_left":
		pSeat->m_bMoveForward = true;
		break;
	case "-attack_left":
		pSeat->m_bMoveForward = false;
		break;
	case "+menu_left":
		break;
	case "-menu_left":
		break;
	case "+attack_right":
		pSeat->m_iInputAttack = true;
		break;
	case "-attack_right":
		pSeat->m_iInputAttack = false;
		break;

	case "+menu_right":
		pSeat->m_iInputReload = true;
		break;
	case "-menu_right":
		pSeat->m_iInputReload = false;
		break;

	/* client alias for a server command */
	case "addon":
		localcmd(sprintf("sv %s\n", sCMD));
		break;
	/* client alias for a client command */
	case "joinTeam":
		localcmd(sprintf("cmd %s\n", sCMD));
		break;


	case "bobup":
		break;
	case "bobdn":
		break;

	case "+commandmenu":
		Commandmenu_Open();
		break;
	case "-commandmenu":
		Commandmenu_Close();
		break;

	/* can be overriden as well */
	default:
		/* developer commands, when the game builds in src/tools */
		if (Tools_ConsoleCommand(sCMD) == true) {
			return (true);
		}

		return HUD_ConsoleCommand(sCMD);
	}
	return (true);
}

void
Cmd_Init(void)
{
	InitStart();

	/* server admin */
	registercommand("addon");

	/* basic actions */
	registercommand("+attack");
	registercommand("-attack");
	registercommand("+attack2");
	registercommand("-attack2");
	registercommand("+reload");
	registercommand("-reload");
	registercommand("+activate");
	registercommand("-activate");
	registercommand("+use");
	registercommand("-use");
	registercommand("+duck");
	registercommand("-duck");
	registercommand("+prone");
	registercommand("-prone");
	registercommand("goprone");
	registercommand("gocrouch");
	registercommand("+gostand");
	registercommand("-gostand");
	registercommand("+sprint");
	registercommand("-sprint");
	registercommand("weapnext");
	registercommand("weapprev");

	/* voting */
	registercommand("vote");
	registercommand("callvote");

	/* teamplay */
	registercommand("joinTeam");

	/* motd */
	registercommand("motd");

	/* hud weapon selection system */
	registercommand("slot1");
	registercommand("slot2");
	registercommand("slot3");
	registercommand("slot4");
	registercommand("slot5");
	registercommand("slot6");
	registercommand("slot7");
	registercommand("slot8");
	registercommand("slot9");
	registercommand("slot10");

	/* commandmenu */
	registercommand("+commandmenu");
	registercommand("-commandmenu");

	/* scoreboard */
	registercommand("+showscores");
	registercommand("-showscores");

	/* meant to be hidden */
	registercommand("addChatLine");

	/* Requested by Slacer */
	registercommand("+zoomin");
	registercommand("-zoomin");

	/* XR binds, temporady until the engine standardizes things */
	registercommand("+attack_left");
	registercommand("-attack_left");
	registercommand("+attack_right");
	registercommand("-attack_right");
	registercommand("+menu_left");
	registercommand("-menu_left");
	registercommand("+menu_right");
	registercommand("-menu_right");


	InitEnd();
}
