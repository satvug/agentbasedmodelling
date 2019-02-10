
struct ButtonState
{
    bool was_down;
    bool is_down;
};

enum KeyCode
{
	button_code_none,
	
	button_code_mouse_left,
	button_code_mouse_right,
	
	button_code_shift,
	button_code_control,

	button_code_backspace = 8,
	button_code_enter = 13,
	
	button_code_delete,

	button_code_arrow_right,
	button_code_arrow_up,
	button_code_arrow_left,
	button_code_arrow_down,
	
	button_code_space = ' ', //32
	button_code_a = 'A', //65
	button_code_b, button_code_c, button_code_d, button_code_e, button_code_f,
	button_code_g, button_code_h, button_code_i, button_code_j, button_code_k,
	button_code_l, button_code_m, button_code_n, button_code_o, button_code_p,
	button_code_q, button_code_r, button_code_s, button_code_t, button_code_u,
	button_code_v, button_code_w, button_code_x, button_code_y, button_code_z,

	button_code_count,
};

struct Input
{
	ButtonState buttons[button_code_count];

	V2 mouse_p;
    V2 mouse_dp;
	f32 mouse_wheel_delta;

	u32 last_pressed_key;
};

#define is_pressed(input, code) ((input)->buttons[(button_code_##code)].is_down)
#define was_pressed(input, code) ((input)->buttons[(button_code_##code)].was_down)
#define just_pressed(input, code) (is_pressed(input, code) && !was_pressed(input, code))
#define just_unpressed(input, code) (!is_pressed(input, code) && was_pressed(input, code))
