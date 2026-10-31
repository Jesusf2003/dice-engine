//
//	cl_viewmodel.qc - first person weapon, drawn by csqc so we own its model and placement.
//	The server only picks the weapon (.weaponmodel), it reaches us as a stat.
//

entity viewgun;

// Live tuning of the weapon's position: "vm_tune_offset "8 5 -6"", "vm_tune_angles "0 3 0"".
var vector autocvar_vm_tune_offset = '0 0 0';	// forward right up, in units
var vector autocvar_vm_tune_angles = '0 0 0';	// pitch yaw roll, in model space

// Called from CSQC_UpdateView in first person only, after Cam_SetupView.
void() Viewmodel_Draw =
{
	float mdl = getstati(STAT_WEAPONMODELI);
	if (!mdl || getstatf(STAT_HEALTH) <= 0)
		return;

	if (!viewgun)
		viewgun = spawn();
	if (viewgun.modelindex != mdl)
		setmodelindex(viewgun, mdl);

	// Placed at the eye by hand: RF_VIEWMODEL left the model invisible with this csqc setup.
	vector ang = getproperty(VF_ANGLES);
	vector ofs = autocvar_vm_tune_offset;
	makevectors(ang);
	viewgun.origin = getproperty(VF_ORIGIN) + v_forward * ofs_x + v_right * ofs_y + v_up * ofs_z;
	viewgun.angles = ang;
	viewgun.angles_x = -viewgun.angles_x;	// models use inverted pitch
	viewgun.angles += autocvar_vm_tune_angles;
	viewgun.renderflags = RF_DEPTHHACK | RF_NOSHADOW;	// depthhack: don't poke into walls
	addentity(viewgun);
};
