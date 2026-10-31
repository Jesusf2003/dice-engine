//
//	cl_input.qc - client side input processing
//

void() Input_Frame =
{
	Cam_Input();

	input_movevalues_x = bound(-350, input_movevalues_x, 350);
	input_movevalues_y = bound(-350, input_movevalues_y, 350);
	input_movevalues_z = bound(-350, input_movevalues_z, 350);
};
