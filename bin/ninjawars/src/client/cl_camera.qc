//
//	cl_camera.qc - places the view every frame, called from CSQC_UpdateView.
//

struct camera_t
{
    float distance;
    float pitch;
};


void() Cam_SetupView =
{
	addentities(MASK_ENGINE);

    float w = 512;
    vector eye = getproperty(VF_ORIGIN);
    vector ang = '30 45 0';

	// Viewmodel_Draw();
    setproperty(VF_PERSPECTIVE, 0);
    setproperty(VF_FOVX, w);
    setproperty(VF_FOVY, w * g_height / g_width);
	setproperty(VF_VIEWENTITY, 0);

    makevectors(ang);
    setproperty(VF_ORIGIN, eye - v_forward * 1000);
    setproperty(VF_ANGLES, ang);
};
