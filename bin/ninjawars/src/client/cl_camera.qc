//
//	cl_camera.qc - first/third person camera, placed every frame by Cam_SetupView.
//

// Console variables (var = not constant, so the engine can change them), can be changed live: "cam_thirdperson 1", or bind c "toggle cam_thirdperson".
var float autocvar_cam_thirdperson = 0;
var float autocvar_cam_dist = 120;		// how far behind the player the camera sits
var float autocvar_cam_side = 0;		// over-the-shoulder offset, positive = right
var float autocvar_cam_up = 8;			// raise the pivot a bit above the eyes
var float autocvar_cam_smooth = 8;		// how fast the camera moves back out after hitting a wall

// The camera is a small box so it can't peek through walls at corners.
const vector CAM_HULL_MINS = '-4 -4 -4';
const vector CAM_HULL_MAXS = '4 4 4';
const float CAM_WALL_MARGIN = 2;

float cam_curdist;	// smoothed distance, 0 in first person

// Places campos/camang behind the eye position, pulled in when something is in the way.
void(vector eye, vector ang) Cam_ThirdPerson =
{
	vector pivot, wanted;
	float allowed;

	makevectors(ang);

	// Pivot: where the camera hangs from. Traced from the eye so the shoulder offset can't go into a wall.
	tracebox(eye, CAM_HULL_MINS, CAM_HULL_MAXS, eye + v_up * autocvar_cam_up + v_right * autocvar_cam_side, MOVE_NOMONSTERS, world);
	if (trace_startsolid)
	{	// the eye itself is too close to a wall, don't try to move the camera
		campos = eye;
		camang = ang;
		cam_curdist = 0;
		return;
	}
	pivot = trace_endpos;

	// Distance actually available behind the pivot.
	wanted = pivot - v_forward * autocvar_cam_dist;
	tracebox(pivot, CAM_HULL_MINS, CAM_HULL_MAXS, wanted, MOVE_NOMONSTERS, world);
	allowed = autocvar_cam_dist * trace_fraction - CAM_WALL_MARGIN;
	if (allowed < 0)
		allowed = 0;

	// Snap in instantly (otherwise it clips through the wall), ease back out.
	if (allowed < cam_curdist)
		cam_curdist = allowed;
	else
		cam_curdist += (allowed - cam_curdist) * min(1, frametime * autocvar_cam_smooth);

	campos = pivot - v_forward * cam_curdist;
	camang = ang;
};

// Called from CSQC_UpdateView right after clearscene: adds the entities and places the view.
void() Cam_SetupView =
{
	vector eye, ang;

	// clearscene fills VF_ORIGIN/VF_ANGLES with the engine's first person view (predicted, with bob).
	eye = getproperty(VF_ORIGIN);
	ang = getproperty(VF_ANGLES);

	addentities(MASK_ENGINE);

	if (autocvar_cam_thirdperson)
	{
		Cam_ThirdPerson(eye, ang);
		setproperty(VF_VIEWENTITY, 0);	// nobody is "us", so our own body gets drawn
		setproperty(VF_ORIGIN, campos);
		setproperty(VF_ANGLES, camang);
	}
	else
	{
		Viewmodel_Draw();
		setproperty(VF_VIEWENTITY, player_localentnum);	// hides our body (still casts shadows)
		campos = eye;
		camang = ang;
		cam_curdist = 0;	// third person slides out from the head when enabled
	}
};
