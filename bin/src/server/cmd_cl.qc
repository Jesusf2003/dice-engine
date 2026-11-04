/*
 * Copyright (c) 2024 Vera Visions LLC.
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

bool
Cmd_ParseClientCommand(ncClient sender, string cmd, int commandArguments)
{
	switch (argv(0)) {
	case "say":
		if (commandArguments == 2) {
			g_grMode.ChatMessageAll(sender, argv(1));
		} else {
			g_grMode.ChatMessageAll(sender, substring(cmd, 4, -1));
		}
		break;
	case "say_team":
		if (commandArguments == 2) {
			g_grMode.ChatMessageTeam(sender, argv(1));
		} else {
			g_grMode.ChatMessageTeam(sender, substring(cmd, 9, -1));
		}
		break;
	case "spectate":
		if (is.Player(sender)) {
			break;
		}

		ncPlayer pl = (ncPlayer)sender;
		pl.MakeSpectator();
		break;
	case "play":
		if (sender.classname != "spectator") {
			break;
		}

		EntityDef_SpawnClassname("player");
		PutClientInServer();
		break;
	case "timeleft":
		string timestring;
		float timeleft;
		timeleft = cvar("timelimit") - (time / 60);
		timestring = Util_TimeToString(timeleft);
		bprint(PRINT_CHAT, sprintf("we have %s minutes remaining", timestring));
		break;
	case "joinTeam":
		g_grMode.PlayerRequestTeam((ncPlayer)sender, (int)stoi(argv(1)));
		break;
	default:
		/* developer commands, when the game builds in src/tools */
		if (Tools_ClientCommand(sender, cmd, commandArguments) == false) {
			clientcommand(sender, cmd);
		}
	}
}
