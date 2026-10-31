//
//	camera.qc - camera library, public API. Include this file only; it pulls in the rest.
//
//	CSQC_Init				Cam_Init(); Cam_SetMode(CAM_...);
//	CSQC_ConsoleCommand		if (Cam_ConsoleCommand()) return TRUE;	(after tokenize_console)
//	CSQC_InputEvent			IE_MOUSEDELTA: return Cam_MouseMove(dx, dy);
//	CSQC_Input_Frame		Cam_Input();
//	CSQC_UpdateView			clearscene(); Cam_SetupView(); ... renderscene();
//
//	Console: "cam_mode" lists the cameras, "cam_mode orbit" (or its number) switches.
//

#include "client/camera/cam_defs.qh"
#include "client/camera/cam_common.qc"
#include "client/camera/cam_firstperson.qc"
#include "client/camera/cam_ortho.qc"
#include "client/camera/cam_thirdperson.qc"
#include "client/camera/cam_orbit.qc"
#include "client/camera/cam_scrollers.qc"

float() Cam_GetMode =
{
	return cam.mode;
};

string(float mode) Cam_ModeName =
{
	switch (mode)
	{
		case CAM_FIRSTPERSON:	return "firstperson";
		case CAM_ORTHO:			return "ortho";
		case CAM_THIRDPERSON:	return "thirdperson";
		case CAM_ORBIT:			return "orbit";
		case CAM_SIDESCROLL:	return "side";
		case CAM_BELTSCROLL:	return "belt";
	}
	return "";
};

// Orbit and the scrollers turn the player towards where it walks; the others leave it to the mouse.
float(float mode) Cam_TurnsPlayer =
{
	return mode == CAM_ORBIT || mode == CAM_SIDESCROLL || mode == CAM_BELTSCROLL;
};

void(float mode) Cam_SetMode =
{
	if (mode < 0 || mode >= CAM_COUNT)
		return;

	// Start from where the player looks now, so switching doesn't spin it around.
	if (!Cam_TurnsPlayer(cam.mode))
		cam.facing = view_angles_y;

	cam.mode = mode;
	cam.distance = 0;
	cam.focus = '0 0 0';
	cam.eyez = 0;
	cam.yaw = cam.facing;
	cam.pitch = mode == CAM_ORBIT ? ORBIT_START_PITCH : 0;
	cam.side_facing = cam.facing;
};

void() Cam_Init =
{
	registercommand("cam_mode", "Lists the cameras, or switches to one by name or number");
};

float() Cam_ConsoleCommand =
{
	if (argv(0) != "cam_mode")
		return FALSE;

	float i;
	if (argv(1) == "")
	{
		for (i = 0; i < CAM_COUNT; i++)
			print(sprintf("%s%g %s\n", i == cam.mode ? "> " : "  ", i, Cam_ModeName(i)));
		return TRUE;
	}

	for (i = 0; i < CAM_COUNT; i++)
		if (argv(1) == Cam_ModeName(i) || argv(1) == ftos(i))
		{
			Cam_SetMode(i);
			return TRUE;
		}

	print(sprintf("cam_mode: unknown camera \"%s\"\n", argv(1)));
	return TRUE;
};

float(float dx, float dy) Cam_MouseMove =
{
	switch (cam.mode)
	{
		case CAM_ORTHO:			return Cam_Ortho_Mouse(dx, dy);
		case CAM_THIRDPERSON:	return Cam_ThirdPerson_Mouse(dx, dy);
		case CAM_ORBIT:			return Cam_Orbit_Mouse(dx, dy);
		case CAM_SIDESCROLL:
		case CAM_BELTSCROLL:	return Cam_Scroller_Mouse(dx, dy);
	}
	return Cam_FirstPerson_Mouse(dx, dy);
};

void() Cam_Input =
{
	switch (cam.mode)
	{
		case CAM_ORBIT:
		case CAM_BELTSCROLL:	Cam_Input_Relative(); break;
		case CAM_SIDESCROLL:	Cam_Input_Side(); break;
		default:				Cam_Input_Look(); break;
	}
};

void() Cam_SetupView =
{
	switch (cam.mode)
	{
		case CAM_ORTHO:			Cam_Ortho_View(); break;
		case CAM_THIRDPERSON:	Cam_ThirdPerson_View(); break;
		case CAM_ORBIT:			Cam_Orbit_View(); break;
		case CAM_SIDESCROLL:	Cam_SideScroll_View(); break;
		case CAM_BELTSCROLL:	Cam_BeltScroll_View(); break;
		default:				Cam_FirstPerson_View(); break;
	}
};
