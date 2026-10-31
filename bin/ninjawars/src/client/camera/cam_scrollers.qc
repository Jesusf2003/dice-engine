//
//	cam_scrollers.qc - side views. Looking along +y, so the level scrolls along x.
//	The side scroller only walks left/right, the belt scroller also into and out of the screen.
//

void() Cam_SideScroll_View =
{
	//               yaw  pitch  distance  height  lookahead  speed
	Cam_ScrollerView(90,  10,    384,      32,     64,        6);
};

void() Cam_BeltScroll_View =
{
	// Looks down more, so the depth of the floor reads.
	Cam_ScrollerView(90,  25,    384,      16,     64,        6);
};

// Nothing to aim in a side view.
float(float dx, float dy) Cam_Scroller_Mouse =
{
	return TRUE;
};
