//
//	cam_ortho.qc - isometric view with first person controls.
//

const float ORTHO_WIDTH = 512;		// visible width of the world, in units
const float ORTHO_DISTANCE = 1000;	// how far back it can go; walls in between pull it in, which cuts them away
const vector ORTHO_ANGLES = '30 45 0';

float(float dx, float dy) Cam_Ortho_Mouse =
{
	return FALSE;
};

void() Cam_Ortho_View =
{
	addentities(MASK_ENGINE);

	vector size = getproperty(VF_SIZE);
	setproperty(VF_PERSPECTIVE, 0);
	setproperty(VF_FOVX, ORTHO_WIDTH);
	setproperty(VF_FOVY, ORTHO_WIDTH * size_y / size_x);
	setproperty(VF_VIEWENTITY, 0);

	makevectors(ORTHO_ANGLES);
	setproperty(VF_ORIGIN, Cam_Boom(Cam_Eye(), -v_forward * ORTHO_DISTANCE));
	setproperty(VF_ANGLES, ORTHO_ANGLES);
};
