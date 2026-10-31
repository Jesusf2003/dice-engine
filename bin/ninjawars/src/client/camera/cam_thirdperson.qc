//
//	cam_thirdperson.qc - over the shoulder, first person controls.
//

const float THIRD_DISTANCE = 96;	// behind the eyes
const float THIRD_SIDE = 16;		// to the right

float(float dx, float dy) Cam_ThirdPerson_Mouse =
{
	return FALSE;
};

void() Cam_ThirdPerson_View =
{
	addentities(MASK_ENGINE);
	setproperty(VF_VIEWENTITY, 0);	// stop hiding our own model

	// The mouse angles, not VF_ANGLES: that one carries the roll, sway and kick meant for first person.
	vector ang = [view_angles_x, view_angles_y, 0];
	makevectors(ang);
	setproperty(VF_ORIGIN, Cam_Boom(Cam_Eye(), v_right * THIRD_SIDE - v_forward * THIRD_DISTANCE));
	setproperty(VF_ANGLES, ang);
};
