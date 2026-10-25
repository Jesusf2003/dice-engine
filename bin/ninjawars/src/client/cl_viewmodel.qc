//
//	cl_viewmodel.qc - first person weapon, drawn by csqc so we own its model, animation and placement.
//	The server only picks the weapon (.weaponmodel / .weaponframe), they reach us as stats.
//

entity viewgun;
float viewgun_shot;

const float VM_BLEND_TIME = 0.08;	// seconds to crossfade from the previous animation

// Called from Cam_SetupView in first person only, after clearscene has set VF_ORIGIN/VF_ANGLES.
void() Viewmodel_Draw =
{
	float mdl = getstati(STAT_WEAPONMODELI);
	if (!mdl || getstatf(STAT_HEALTH) <= 0)
		return;

	if (!viewgun)
		viewgun = spawn();
	if (viewgun.modelindex != mdl)
		setmodelindex(viewgun, mdl);

	// weaponframe is a framegroup (idle, fire, ...). csqc doesn't autoanimate them, so we advance the time ourselves.
	// A new shot restarts the animation even when the group is the same (holding attack).
	float group = getstatf(STAT_WEAPONFRAME);
	float shot = getstatf(STAT_WEAPONSHOT);
	if (group != viewgun.frame || shot != viewgun_shot)
	{
		// The old pose moves to frame2 and fades out, so switches don't pop.
		viewgun.frame2 = viewgun.frame;
		viewgun.frame2time = viewgun.frame1time;
		viewgun.lerpfrac = 1;
		viewgun.frame = group;
		viewgun.frame1time = 0;
		viewgun_shot = shot;
	}
	else
	{
		viewgun.frame1time += frametime;
		viewgun.frame2time += frametime;
		viewgun.lerpfrac = max(0, viewgun.lerpfrac - frametime / VM_BLEND_TIME);
	}

	// Placed at the eye by hand: RF_VIEWMODEL left the model invisible with this csqc setup.
	// Weapon models are authored with the eye at their origin, so no offset is needed.
	viewgun.origin = getproperty(VF_ORIGIN);
	viewgun.angles = getproperty(VF_ANGLES);
	viewgun.angles_x = -viewgun.angles_x;	// models use inverted pitch
	viewgun.renderflags = RF_DEPTHHACK | RF_NOSHADOW;	// depthhack: don't poke into walls
	addentity(viewgun);
};
