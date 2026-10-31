//
//	cam_orbit.qc - the mouse turns the camera around the player, the player faces where it walks.
//

const float ORBIT_DISTANCE = 128;
const float ORBIT_START_PITCH = 20;	// starts a bit above, so the player doesn't cover the middle of the screen
const float ORBIT_PITCH_LIMIT = 80;	// stops before going over the top or under the feet

float(float dx, float dy) Cam_Orbit_Mouse =
{
	float scale = cvar("sensitivity");
	cam.yaw -= dx * scale * cvar("m_yaw");
	cam.pitch = bound(-ORBIT_PITCH_LIMIT, cam.pitch + dy * scale * cvar("m_pitch"), ORBIT_PITCH_LIMIT);
	return TRUE;
};

void() Cam_Orbit_View =
{
	Cam_ShowFacing();
	addentities(MASK_ENGINE);
	setproperty(VF_VIEWENTITY, 0);	// stop hiding our own model

	vector ang = [cam.pitch, cam.yaw, 0];
	makevectors(ang);
	setproperty(VF_ORIGIN, Cam_Boom(Cam_Eye(), -v_forward * ORBIT_DISTANCE));
	setproperty(VF_ANGLES, ang);
};
