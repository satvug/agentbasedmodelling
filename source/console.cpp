
void console_init(ProfessionalQualityConsole *console, f32 h, V4 panel_color, f32 time_to_open)
{
	f32 w = 1;
	console->panel.dim_from = v2(w, h);
	console->panel.dim_to = v2(w, h);
	console->panel.p_from = v2(0.5f * w, 1 + 0.5f * h);
	console->panel.p_to = v2(0.5f * w, 1 - 0.5f * h);
	console->panel.color_from = panel_color;	
	console->panel.color_to = panel_color;
	console->panel.time_to_open = time_to_open;
	console->panel.default_openness = 0.0f;
	console->panel.min_openness = 0.0f;
	console->panel.max_openness = 1.0f;
	console->panel.target_openness = console->panel.default_openness;
	console->panel.current_openness = console->panel.default_openness;

	console->history.data = console->history_buffer;
	console->history.length = 0;
	console->input.data = console->input_buffer;
	console->input.length = 0;

	console->open = false;
}

void toggle_console(ProfessionalQualityConsole *console)
{
	console->open = !console->open;
	ProfessionalQualityUIPanel *panel = &console->panel;
	panel->target_openness = console->open ? panel->max_openness : panel->min_openness;
}

inline void clear_console(ProfessionalQualityConsole *console)
{
	console->history_line_count = 0;
	console->history.length = 0;
	console->input.length = 0;
}

// TODO: add the String struct handling into our sprintf
// TODO: handle the buffer overflow
// NOTE: Just the most common pattern for us here
inline void console_append_line(ProfessionalQualityConsole *console, char *prefix, String string, char *postfix)
{
	u32 max_char_count = sizeof(console->history_buffer);
	append(&console->history, prefix, max_char_count);
	append(&console->history, string, max_char_count);
	append(&console->history, postfix, max_char_count);
	++console->history_line_count;
}

inline void console_append_line(ProfessionalQualityConsole *console, char *prefix, String string)
{
	u32 max_char_count = sizeof(console->history_buffer);
	append(&console->history, prefix, max_char_count);
	append(&console->history, string, max_char_count);
	++console->history_line_count;
}

inline void console_append_line(ProfessionalQualityConsole *console, String string, char *postfix)
{
	u32 max_char_count = sizeof(console->history_buffer);
	append(&console->history, string, max_char_count);
	append(&console->history, postfix, max_char_count);
	++console->history_line_count;
}

inline void console_append_line(ProfessionalQualityConsole *console, String line)
{
	u32 max_char_count = sizeof(console->history_buffer);
	append(&console->history, line, max_char_count);
	++console->history_line_count;
}

inline void console_append_line(ProfessionalQualityConsole *console, char *line)
{
	u32 max_char_count = sizeof(console->history_buffer);
	append(&console->history, string(line), max_char_count);
	++console->history_line_count;
}

void execute_current_command(SimStorage *storage, RenderInfo *render_info, ProfessionalQualityConsole *console)
{
	String input = eat_whitespace(console->input);
	String command = eat_word(input);
	if(equal(command, "clear"))
	{
		clear_console(console);
	}
	else if(equal(command, "close"))
	{
		console->open = false;
		ProfessionalQualityUIPanel *panel = &console->panel;
		panel->target_openness = panel->min_openness;
	}
	else if(equal(command, "load"))
	{
		String path = eat_whitespace(advance(input, command.length));
		if(path.length > 0)
		{
			set_default_sim_params(&storage->state);
			StatusCode status_code = load_environment(storage, path);
			if(status_code == status_code_success)
			{
				fit_sim_camera(&storage->state, render_info);
				storage->editor.debug_camera = storage->state.sim_camera;
				
				console_append_line(console, "loaded environment from '", path, "'\n");
				storage->last_successful_load_path.length = 0;
				append(&storage->last_successful_load_path, path, sizeof(storage->last_successful_load_path_buffer));
			}
			else
			{
				init_default_environment(storage);
				fit_sim_camera(&storage->state, render_info);
				char *status_msg = status_code == status_code_invalid_path ? "' : invalid path\n" : "' : unknown specifier\n";
				console_append_line(console, "ERROR: Couldn't load environment from '", path, status_msg);
			    console_append_line(console, "loaded default environment\n");
			}
		}
		else
		{
			console_append_line(console, "ERROR: No path specified\n");
		}
	}
	else if(equal(command, "reload"))
	{
		if(storage->last_successful_load_path.length > 0)
		{
			set_default_sim_params(&storage->state);
			reload_environment(storage);
			fit_sim_camera(&storage->state, render_info);
			storage->editor.debug_camera = storage->state.sim_camera;
			console_append_line(console, "environment loaded from the last valid path\n");
		}
		else
		{
			console_append_line(console, "ERROR: No successful load information found\n");
		}
	}
	else if(equal(command, "load_default"))
	{
		init_default_environment(storage);
		fit_sim_camera(&storage->state, render_info);
		console_append_line(console, "loaded default environment\n");
	}
	else if(equal(command, "save"))
	{
		input = advance(input, command.length);
		String path = eat_word(eat_whitespace(input));
		if(path.length > 0)
		{
			StatusCode status_code = save_environment(storage, path);
			if(status_code == status_code_success)
			{
				console_append_line(console, "environment saved to '", path, "'\n");
			}
			else
			{
				console_append_line(console, "ERROR: Couldn't save environment to '", path, "' : invalid path\n");
			}
		}
		else
		{
			console_append_line(console, "ERROR: No path specified\n");
		}
	}
	else if(equal(command, "dump"))
	{
		input = advance(input, command.length);
		String path = eat_word(eat_whitespace(input));
		if(path.length > 0)
		{
			StatusCode status_code = dump_stats(storage, path);
			if(status_code == status_code_success)
			{
				console_append_line(console, "stats dumped to '", path, "'\n");
			}
			else
			{
				console_append_line(console, "ERROR: Couldn't dump stats to '", path, "' : invalid path\n");
			}
		}
		else
		{
			console_append_line(console, "ERROR: No path specified\n");
		}
	}
	else if(equal(command, "spawn"))
	{
		input = advance(input, command.length);
		String arg = eat_word(eat_whitespace(input));
		if(arg.length > 0)
		{
			s32 count;
			bool success = string_to_integer(arg, &count);
			if(success && count > 0)
			{
				storage->state.last_requested_spawn_count = count;
				s32 spawned_count = spawn_random_agents(storage, count);
				if(spawned_count == count)
				{
					console_append_line(console, arg, " agents spawned\n");
				}
				else
				{
					console_append_line(console, "WARNING: Couldn't spawn specified number of agents\n");
				}
			}
			else
			{
				console_append_line(console, "ERROR: Invalid number of agents\n");
			}
		}
		else
		{
			console_append_line(console, "ERROR: No agent count specified\n");
		}
	}
	else if(equal(command, "run"))
	{
		input = advance(input, command.length);
		String arg = eat_word(eat_whitespace(input));
		if(arg.length > 0)
		{
			s32 index;
			bool success = string_to_integer(arg, &index);
			if(success && index >= 0 && index < (s32)storage->run_count)
			{
				set_run_index(storage, index);
				console_append_line(console, arg, " run index set\n");
			}
			else
			{
				console_append_line(console, "ERROR: Invalid run index\n");
			}
		}
		else
		{
			console_append_line(console, "ERROR: No run index specified\n");
		}
	}
	/*
	else if(equal(command, "set"))
	{
		input = advance(input, command.length);
		String arg = eat_word(eat_whitespace(input));
		if(arg.length > 0)
		{
			input = advance(input, (u32)(arg.data - input.data) + arg.length);
			if(equal(arg, "radius"))
			{
				arg = eat_word(eat_whitespace(input));
				f32 value;
				if(string_to_float(arg, &value))
				{
					storage->state.params.radius = value;
				}
			}
			else if(equal(arg, "desired_velocity"))
			{
				arg = eat_word(eat_whitespace(input));
				f32 value;
				if(string_to_float(arg, &value))
				{
					storage->state.params.desired_velocity = value;
				}
			}
			else
			{
				console_append_line(console, "ERROR: Unknown variable name\n");
			}
		}
		else
		{
			console_append_line(console, "ERROR: No variable name specified\n");
		}
	}
	*/
	else
	{
		console_append_line(console, "ERROR: Unknown command: '", command, "'\n");
	}
}

void update_console(SimStorage *storage, RenderInfo *render_info, ProfessionalQualityConsole *console, char character)
{
	if(character >= ' ' && character <= '~')
	{
		append(&console->input, character, sizeof(console->input_buffer));
	}
	else if(console->input.length > 0 && character == button_code_backspace)
	{
		--console->input.length;
	}
	else if(console->input.length > 0 && character == button_code_enter)
	{
		u32 new_char_count = console->history.length + console->input.length + 3;
		u32 max_char_count = sizeof(console->history_buffer);
		if(new_char_count < max_char_count)
		{
			console_append_line(console, "> ", console->input, "\n");
		}
		else
		{
		}
		execute_current_command(storage, render_info, console);
	    console->input.length = 0;
	}
}

//
// NOTE: Still very ad hoc console rendering routine
// not that it's any relevant while the console does the things you'd expect it to do
// also, text being wider than the console is not handled at all, but it should be much easier to
// fix once the text rendering becomes more advanced (e.g. different horizontal/vertical alignments)
void render_console(ProfessionalQualityConsole *console, RenderInfo *render_info)
{
	ProfessionalQualityUIPanel *panel = &console->panel;

	Rect2 view_rect = get_camera_rect(render_info->current_camera);
	Rect2 panel_rect = get_professional_quality_ui_panel_rect(panel, view_rect);
	Rect2 input_field_rect = panel_rect;
	input_field_rect.max.y = input_field_rect.min.y + 0.1f * get_height(panel_rect);
	
	V4 panel_color = get_professional_quality_ui_panel_color(panel);
	V4 input_field_color = 0.5f * panel_color;
	input_field_color.a = panel_color.a;
	
	push_rect(render_info, panel_rect, panel_color, rendering_priority_ui_base + 100);
	push_rect(render_info, input_field_rect, input_field_color, rendering_priority_ui_base + 101);
	
	V4 input_color = v4(0.8f, 0.8f, 0.8f, 1.0f);
	f32 input_x_min = 0.01f;
	f32 input_y_min = 0.30f;
	f32 input_y_max = 1.00f;
	V2 input_min_corner = get_rect_relative_p(input_field_rect, v2(input_x_min, input_y_min));
	f32 input_line_height = (input_y_max - input_y_min) * get_height(input_field_rect);
	V2 at = input_min_corner;

	f32 x_advance;
	x_advance = get_text_width(&render_info->assets.debug_font, "> ", input_line_height);
	push_string(render_info, "> ", at, input_line_height, input_color, rendering_priority_ui_base + 102);
    at.x += x_advance;

	x_advance = get_text_width(&render_info->assets.debug_font, console->input, input_line_height);
    push_string(render_info, console->input, at, input_line_height, input_color, rendering_priority_ui_base + 102);
	at.x += x_advance;

	f32 input_cursor_height = input_line_height;
	f32 input_cursor_width = 0.4f * input_cursor_height;
	f32 input_cursor_x = at.x;
	f32 input_cursor_y = input_min_corner.y - 0.2f * input_cursor_height;
	Rect2 cursor_rect = min_dim_rect(input_cursor_x, input_cursor_y, input_cursor_width, input_cursor_height);
	push_rect(render_info, cursor_rect, input_color, rendering_priority_ui_base + 102);

	Font *font = &render_info->assets.debug_font;
	f32 history_line_height = input_line_height * font->line_step / font->height;
	f32 history_height = (console->history_line_count - 1) * history_line_height;
	V2 history_min_corner = {input_min_corner.x, input_field_rect.max.y + 0.4f * history_line_height + history_height};
    push_string(render_info, console->history, history_min_corner, input_line_height, input_color, rendering_priority_ui_base + 102);
}
