//
//	cam_common.qc - pieces shared by several cameras: smoothing, wall collision, ladders and the input styles.
//

const float CAM_WALL_GAP = 4;		// keeps the near plane from poking through walls
const float CAM_RETURN_SPEED = 8;	// how fast a camera slides back out after a wall
const float CAM_STEP_SPEED = 15;	// how fast the eye height follows stairs
const vector CAM_HULL = '6 6 6';	// half size of the box traced for the camera, so walls beside it don't clip either

// Fraction of the remaining way to cover this frame, to ease towards a target at `speed`.
float(float speed) Cam_Ease =
{
	return min(1, clframetime * speed);
};

// The player's eyes from the prediction, without the bob, kick and sway clearscene puts in VF_ORIGIN
// for first person. Stairs are smoothed, big jumps (spawn, teleport) snap.
vector() Cam_Eye =
{
	float height = getstatf(STAT_VIEWHEIGHT);
	vector eye = pmove_org + '0 0 1' * (height ? height : 22);

	if (fabs(eye_z - cam.eyez) > 64)
		cam.eyez = eye_z;
	else
		cam.eyez += (eye_z - cam.eyez) * Cam_Ease(CAM_STEP_SPEED);
	eye_z = cam.eyez;
	return eye;
};

// Camera position `offset` away from `eye`, stopping before the map. Snaps in on walls, eases back out.
vector(vector eye, vector offset) Cam_Boom =
{
	tracebox(eye, -CAM_HULL, CAM_HULL, eye + offset, MOVE_NOMONSTERS, world);
	float target = trace_fraction * vlen(offset);

	if (target < cam.distance)
		cam.distance = target;
	else
		cam.distance += (target - cam.distance) * Cam_Ease(CAM_RETURN_SPEED);

	return eye + normalize(offset) * max(0, cam.distance - CAM_WALL_GAP);
};

// The engine draws our own model with its view angles, so cameras that turn the player put the facing there.
void() Cam_ShowFacing =
{
	setproperty(VF_CL_VIEWANGLES, [0, cam.facing, 0]);
};

float() Cam_OnFloor =
{
	tracebox(pmove_org, PLAYER_MINS, PLAYER_MAXS, pmove_org - '0 0 2', MOVE_NOMONSTERS, world);
	return trace_fraction < 1;
};

// On a ladder forward/back become straight up/down, whatever the camera.
// TRUE means it took the keys: leave the facing alone, turning away from the wall lets go.
float() Cam_LadderClimb =
{
	if (input_movevalues_y)
		return FALSE;	// strafing steps off sideways

	if (getstatf(STAT_PMOVE_FLAGS) & PMF_LADDER)
	{
		// At the bottom, back walks away instead of pressing into the floor.
		if (input_movevalues_x < 0 && Cam_OnFloor())
			return FALSE;

		cam.ladder_time = time;

		// The engine climbs along the view's forward and up axes, which tilt with the pitch. Splitting the
		// climb between both cancels the tilt, so the view doesn't need to change (it would jerk in first person).
		float climb = input_movevalues_x;
		float pitch = input_angles_x * (M_PI / 180);
		input_movevalues_x = -climb * sin(pitch);
		input_movevalues_z = climb * cos(pitch);
		return TRUE;
	}

	// Just past the top: keep walking forward, onto the ledge.
	return input_movevalues_x > 0 && time - cam.ladder_time < 0.5;
};

// First person controls: the mouse turns the player, the keys move where it looks.
void() Cam_Input_Look =
{
	Cam_LadderClimb();
};

// The keys move relative to cam.yaw and the player turns towards where it walks.
void() Cam_Input_Relative =
{
	if (!Cam_LadderClimb() && (input_movevalues_x || input_movevalues_y))
	{
		makevectors([0, cam.yaw, 0]);
		vector wish = v_forward * input_movevalues_x + v_right * input_movevalues_y;
		cam.facing = vectoyaw(wish);
		// Same speed in every direction: diagonals aren't faster, forward isn't slower than strafing.
		input_movevalues_x = max(fabs(input_movevalues_x), fabs(input_movevalues_y));
		input_movevalues_y = 0;
	}
	input_angles = [0, cam.facing, 0];
};

// Only left/right along the screen. Holding forward turns the player to the back, so it can grab a ladder there.
void() Cam_Input_Side =
{
	if (!Cam_LadderClimb())
	{
		if (input_movevalues_y)
		{
			cam.side_facing = input_movevalues_y > 0 ? cam.yaw - 90 : cam.yaw + 90;
			cam.facing = cam.side_facing;
			input_movevalues_x = fabs(input_movevalues_y);
			input_movevalues_y = 0;
		}
		else
		{
			cam.facing = input_movevalues_x > 0 ? cam.yaw : cam.side_facing;
			input_movevalues_x = 0;
		}
	}
	input_angles = [0, cam.facing, 0];
};

// Side view that follows the player from `distance` away, aiming `height` above the eyes and `lookahead`
// ahead of where it faces (only along the screen), easing after it at `speed`.
void(float yaw, float pitch, float distance, float height, float lookahead, float speed) Cam_ScrollerView =
{
	Cam_ShowFacing();
	addentities(MASK_ENGINE);
	setproperty(VF_VIEWENTITY, 0);	// stop hiding our own model

	cam.yaw = yaw;
	cam.pitch = pitch;

	vector eye = Cam_Eye();
	makevectors([0, cam.facing, 0]);
	vector facing = v_forward;
	makevectors([0, yaw, 0]);
	vector focus = eye + v_right * (facing * v_right) * lookahead + '0 0 1' * height;

	if (cam.focus == '0 0 0')
		cam.focus = focus;	// first frame: start in place, not sliding in from the map origin
	else
		cam.focus += (focus - cam.focus) * Cam_Ease(speed);

	vector ang = [pitch, yaw, 0];
	makevectors(ang);
	// Traced from the eyes, not the focus: the lookahead can put the focus inside a wall.
	setproperty(VF_ORIGIN, Cam_Boom(eye, cam.focus - v_forward * distance - eye));
	setproperty(VF_ANGLES, ang);
};
