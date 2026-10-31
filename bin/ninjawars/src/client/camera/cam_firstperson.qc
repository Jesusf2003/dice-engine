//
//	cam_firstperson.qc - the engine's own view from the eyes. The game draws its viewmodel on top.
//

float(float dx, float dy) Cam_FirstPerson_Mouse =
{
	return FALSE;
};

void() Cam_FirstPerson_View =
{
	addentities(MASK_ENGINE);
};
