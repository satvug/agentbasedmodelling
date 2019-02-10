
void update_professional_quality_ui_panel_openness(ProfessionalQualityUIPanel *panel, f32 dt)
{
	f32 target = panel->target_openness;
	f32 current = panel->current_openness;
	if(target != current)
	{
		f32 openness_increment = dt / panel->time_to_open;

		if(target > current)
		{
		    current += openness_increment;
			if(target < current)
			{
			    current = target;
			}
		}
		else if(target < current)
		{
		    current -= openness_increment;
			if(target > current)
			{
			    current = target;
			}
		}
		panel->current_openness = current;
	}
}

V2 get_camera_rect_offset(Camera2 *camera, V2 v, f32 vertical_apron_percentage = 0)
{
	f32 apron = vertical_apron_percentage * camera->view_dim.y;
	Rect2 boundary = get_camera_rect(camera);
	boundary.min.x += apron;
	boundary.min.y += apron;
	boundary.max.x -= apron;
	boundary.max.y -= apron;
	
	V2 result;
	result.x = v.x > boundary.max.x ? (v.x - boundary.max.x) :
		(v.x < boundary.min.x ? (v.x - boundary.min.x) : 0.0f);
	result.y = v.y > boundary.max.y ? (v.y - boundary.max.y) :
		(v.y < boundary.min.y ? (v.y - boundary.min.y) : 0.0f);

	return result;
}

void editor_init(SimStorage *storage)
{
	SimEditor *editor = &storage->editor;
	editor->vertical_apron_percentage = 0.03f;
	editor->snapping_grid_dim = 0.025f;
	editor->snapping_range_percentage = 0.01f;
    editor->camera_sensitivity = 0.2f;
	editor->selected_mid_waypoint_index = -1;

	f32 w = 0.1f;
	f32 h = 1.0f;
	editor->panel.dim_from = v2(w, h);
	editor->panel.dim_to = v2(w, h);
	editor->panel.p_from = v2(1 + 0.5f * w, 0.5f * h);
	editor->panel.p_to = v2(1 - 0.5f * w, 0.5f * h);
	editor->panel.color_from = v4(0.2f, 0.2f, 0.2f, 0.5f);	
	editor->panel.color_to = v4(0.2f, 0.2f, 0.2f, 1.0f);
	editor->panel.time_to_open = 0.5f;
	editor->panel.default_openness = 0.2f;
	editor->panel.min_openness = 0.0f;
	editor->panel.max_openness = 1.0f;
	editor->panel.target_openness = editor->panel.default_openness;
	editor->panel.current_openness = editor->panel.default_openness;
}

inline void set_edit_mode(SimEditor *editor, EditMode mode)
{
	editor->last_edit_mode = editor->current_edit_mode;
	editor->current_edit_mode = mode;
	editor->is_drawing_line = false;
}

Rect2 get_professional_quality_ui_panel_rect(ProfessionalQualityUIPanel *panel, Rect2 view_rect)
{
	V2 screen_dim = lerp(panel->dim_from, panel->current_openness, panel->dim_to);
	V2 world_dim = screen_dim * get_dim(view_rect);
	V2 screen_p = lerp(panel->p_from, panel->current_openness, panel->p_to);
	V2 world_p = get_rect_relative_p(view_rect, screen_p);

	Rect2 result = center_dim_rect(world_p, world_dim);
	return result;
}

V4 get_professional_quality_ui_panel_color(ProfessionalQualityUIPanel *panel)
{
	V4 result = lerp(panel->color_from, panel->current_openness, panel->color_to);
	return result;
}

inline void calculate_snapping(V2 snappint_point, V2 p, f32 snapping_range,
							   f32 *closest_distances, f32 *closest_points)
{
	for(u32 i = 0; i < 2; ++i)
	{
		f32 distance = fabs(snappint_point.e[i] - p.e[i]);
		if(distance < snapping_range && distance < closest_distances[i])
		{
			closest_distances[i] = distance;
			closest_points[i] = snappint_point.e[i];
		}
	}
}

inline V2 snap(SimStorage *storage, V2 cursor_p, RenderInfo *render_info)
{
	// NOTE: This snapping calculation code is a sin against humanity.
	// This abomination just should not exist.

	SimEditor *editor = &storage->editor;
	Rect2 view_rect = get_camera_rect(render_info->current_camera);
	f32 line_thickness = get_height(view_rect) * DEBUG_LINE_THICKNESS / DEFAULT_VIEW_HEIGHT;
	
	f32 inf = infinity();
	f32 snapping_range = editor->debug_camera.view_dim.y * editor->snapping_range_percentage;

	V2 closest_distance = {inf, inf};
	V2 closest_point = {};

	V4 grid_color = v4(0.8f, 0.8f, 0.8f, 0.7f);
	Rect2 sim_boundary = storage->state.sim_boundary;
	f32 snapping_grid_dim = editor->snapping_grid_dim * get_height(sim_boundary);
	V2 grid_p = sim_boundary.min;// + snapping_grid_dim * v2(0.5f, 0.5f);
			
	while(grid_p.x <= sim_boundary.max.x)
	{
		push_line(render_info, v2(grid_p.x, sim_boundary.min.y), v2(grid_p.x, sim_boundary.max.y),
				  0.6f * line_thickness, grid_color, rendering_priority_sim_background);
		calculate_snapping(v2(grid_p.x, grid_p.y), cursor_p, snapping_range, closest_distance.e, closest_point.e);
		grid_p.x += snapping_grid_dim;
	}
			
	while(grid_p.y <= sim_boundary.max.y)
	{
		push_line(render_info, v2(sim_boundary.min.x, grid_p.y), v2(sim_boundary.max.x, grid_p.y),
				  0.6f * line_thickness, grid_color, rendering_priority_sim_background);
		calculate_snapping(v2(grid_p.x, grid_p.y), cursor_p, snapping_range, closest_distance.e, closest_point.e);
		grid_p.y += snapping_grid_dim;
	}
			
	if(editor->is_drawing_line)
	{
		calculate_snapping(editor->current_line_p0, cursor_p, snapping_range, closest_distance.e, closest_point.e);
	}
			
	//for(u32 i = 0; i < storage->obstacle_count; ++i)
	//{
	//	calculate_snapping(storage->obstacles[i].points[0], cursor_p, snapping_range, closest_distance.e, closest_point.e);
	//	calculate_snapping(storage->obstacles[i].points[1], cursor_p, snapping_range, closest_distance.e, closest_point.e);
	//}

	s16 waypoint_count = storage->target_waypoint_count + storage->intermediate_waypoint_count;
	for(s16 i = 0; i < waypoint_count; ++i)
	{
		calculate_snapping(storage->waypoints[i], cursor_p, snapping_range, closest_distance.e, closest_point.e);
	}

	V2 result = cursor_p;
	
	if(closest_distance.x < inf)
	{
		result.x = closest_point.x;
		push_line(render_info, v2(result.x, view_rect.min.y), v2(result.x, view_rect.max.y),
				  0.7f * line_thickness, color_green, rendering_priority_sim_background);
	}
			
	if(closest_distance.y < inf)
	{
		result.y = closest_point.y;
		push_line(render_info, v2(view_rect.min.x, result.y), v2(view_rect.max.x, result.y),
				  0.7f * line_thickness, color_green, rendering_priority_sim_background);
	}

	return result;
}

void update_editor(SimStorage *storage, RenderInfo *render_info, Input *input, f32 dt)
{
	SimEditor *editor = &storage->editor;
	
	f32 debug_line_thickness = render_info->current_camera->view_dim.y * DEBUG_LINE_THICKNESS / DEFAULT_VIEW_HEIGHT;
	set_view_dim(render_info, render_info->current_camera->view_dim * (1.0f - 0.1f * input->mouse_wheel_delta));
		
	V2 cursor_p = unproject(input->mouse_p, render_info);
	Rect2 view_rect = get_camera_rect(render_info->current_camera);

	ProfessionalQualityUIPanel *panel = &editor->panel;
    update_professional_quality_ui_panel_openness(panel, dt);
	
	Rect2 panel_rect = get_professional_quality_ui_panel_rect(panel, view_rect);
	V4 panel_color = get_professional_quality_ui_panel_color(panel);

	V2 button0_p = get_rect_relative_p(panel_rect, v2(0.5f, 0.75f));
	V2 button1_p = get_rect_relative_p(panel_rect, v2(0.5f, 0.50f));
	V2 button2_p = get_rect_relative_p(panel_rect, v2(0.5f, 0.25f));
	
	f32 button_r = 0.25f * get_width(panel_rect);

	f32 circle_icon_r = 0.7f * button_r;
	f32 line_icon_r = 0.5f * button_r;
	f32 line_icon_thickness = 0.2f * button_r;
	f32 button_outline_thickness = 0.05f * button_r;
	
	V4 button_color = v4(0.1f, 0.1f, 0.1f, 0.5f);
	V4 icon_color = v4(0.8f, 0.8f, 0.8f, 0.5f);
	
	V4 button0_color = button_color, button1_color = button_color, button2_color = button_color;
	V4 button0_icon_color = icon_color, button1_icon_color = icon_color, button2_icon_color = icon_color;
	V4 button0_outline_color = {}, button1_outline_color = {}, button2_outline_color = {};
	if(editor->current_edit_mode == edit_mode_agent_placement)
	{
		button0_color.a = button0_icon_color.a = panel_color.a;
		button0_outline_color = button0_icon_color;
	}
	else if(editor->current_edit_mode == edit_mode_obstacle_placement)
	{
		button1_color.a = button1_icon_color.a = panel_color.a;
		button1_outline_color = button1_icon_color;
	}
	else if(editor->current_edit_mode == edit_mode_waypoint_placement)
	{
		button2_color.a = button2_icon_color.a = panel_color.a;
		button2_outline_color = button2_icon_color;
	}
	
	if(intersect(panel_rect, cursor_p))
	{
		panel->target_openness = panel->max_openness;
		if(intersect(button0_p, button_r, cursor_p))
		{
			button0_color.a = panel_color.a;
			button0_icon_color.a = panel_color.a;
			button0_outline_color = button0_icon_color;
			if(just_pressed(input, mouse_left))
			{
				set_edit_mode(editor, editor->current_edit_mode == edit_mode_agent_placement ?
							  edit_mode_none : edit_mode_agent_placement);
			}
		}
		else if(intersect(button1_p, button_r, cursor_p))
		{
			button1_color.a = panel_color.a;
			button1_icon_color.a = panel_color.a;
			button1_outline_color = button1_icon_color;
			if(just_pressed(input, mouse_left))
			{
				set_edit_mode(editor, editor->current_edit_mode == edit_mode_obstacle_placement ?
							  edit_mode_none : edit_mode_obstacle_placement);
			}
		}
		else if(intersect(button2_p, button_r, cursor_p))
		{
			button2_color.a = panel_color.a;
			button2_icon_color.a = panel_color.a;
			button2_outline_color = button2_icon_color;
			if(just_pressed(input, mouse_left))
			{
				set_edit_mode(editor, editor->current_edit_mode == edit_mode_waypoint_placement ?
							  edit_mode_none : edit_mode_waypoint_placement);
			}
		}
	}
	else
	{
		panel->target_openness = panel->default_openness;
		if(is_pressed(input, control))
		{
			if(just_pressed(input, s))
			{
				storage->state.sim_camera = storage->editor.debug_camera;
			}
		}
		if(!editor->is_drawing_line && is_pressed(input, mouse_right))
		{
			if(!was_pressed(input, mouse_right))
			{
				editor->world_anchor = cursor_p;
			}
			move_camera(render_info, editor->world_anchor - cursor_p);
		}

		// NOTE: snap to grid/existing objects
		V2 p = is_pressed(input, shift) ? snap(storage, cursor_p, render_info) : cursor_p;

		switch(editor->current_edit_mode)
		{
			case edit_mode_none:
			{
				// TODO: seletion, copy/pasting and deletion goes here, waypoint placement should be separate
				if(just_pressed(input, mouse_left))
				{
					bool selected = false;
					for(u32 i = 0; i < storage->agent_count && !selected; ++i)
					{
						Agent *agent = storage->agents + i;
					    if(intersect(agent->p, agent->radius, p))
						{
							if(storage->selected_agent == agent->id)
							{
								storage->selected_agent.value = 0;
							}
							else
							{
								storage->selected_agent = agent->id;
							}
							selected = true;
						}
					}
					
					if(!selected)
					{
						for(s16 i = 0; i < storage->target_waypoint_count; ++i)
						{
							V2 waypoint = storage->waypoints[i];
							f32 radius = 0.8f * DEFAULT_AGENT_RADIUS;
						    if(intersect(waypoint, radius, p))
							{
								if(storage->selected_target_index == i)
								{
									storage->selected_target_index = -1;
								}
								else
								{
									storage->selected_target_index = i;
							    }
								break;
							}
						}
					}
				}
				break;
			}
		
			case edit_mode_agent_placement:
			{
				f32 r = DEFAULT_AGENT_RADIUS;
				if(is_pressed(input, mouse_left))
				{
					if(!was_pressed(input, mouse_left) || is_pressed(input, r))
					{
						bool free_space = true;
						for(u32 i = 0; i < storage->agent_count && free_space; ++i)
						{
							Agent *other_agent = storage->agents + i;
							free_space = !intersect(p, r, other_agent->p, other_agent->radius);
						}
						if(free_space)
						{
							place_agent(storage, default_agent(storage), p);
						}
					}
				}
				push_circle(render_info, p, r, color_white * v4(1, 1, 1, 0.5), rendering_priority_sim_overlay);			
				break;
			}
		
			case edit_mode_obstacle_placement:
			{
				f32 r = DEFAULT_OBSTACLE_THICKNESS;
				if(just_pressed(input, mouse_left))
				{
					if(editor->is_drawing_line)
					{
						add_obstacle_line(storage, editor->current_line_p0, p);
						update_environment_graph(storage);
					}
					editor->is_drawing_line = true;
					editor->current_line_p0 = p;
				}
				else if(just_pressed(input, mouse_right))
				{
					editor->world_anchor = cursor_p;
					editor->is_drawing_line = false;
				}
				push_circle(render_info, p, r, color_white * v4(1, 1, 1, 0.5), rendering_priority_sim_overlay);
				if(editor->is_drawing_line)
				{
					push_line(render_info, editor->current_line_p0, p, DEFAULT_OBSTACLE_THICKNESS);
				}
				break;
			}

			case edit_mode_waypoint_placement:
			{
				bool target = is_pressed(input, control);
				if(just_pressed(input, mouse_left))
				{
					s16 selected_waypoint_index = -1;
					s16 waypoint_count = storage->target_waypoint_count + storage->intermediate_waypoint_count;
					for(s16 i = 0; i < waypoint_count; ++i)
					{
						V2 waypoint = storage->waypoints[i];
						f32 radius = 0.8f * DEFAULT_AGENT_RADIUS;
						if(intersect(waypoint, radius, p))
						{
							selected_waypoint_index = i;
							break;
						}
					}
					
					if(selected_waypoint_index < 0)
					{
						add_waypoint(storage, p, target);
						update_environment_graph(storage);
					}
					else
					{
						if(editor->selected_mid_waypoint_index == -1)
						{
							if(selected_waypoint_index >= storage->target_waypoint_count)
							{
								editor->selected_mid_waypoint_index = selected_waypoint_index;
							}
						}
						else
						{
							if(editor->selected_mid_waypoint_index == selected_waypoint_index)
							{
								editor->selected_mid_waypoint_index = -1;
							}
							else if(storage->selected_target_index >= 0)
							{
								storage->shortest_path_indices[storage->selected_target_index]
									[editor->selected_mid_waypoint_index] = selected_waypoint_index;
								editor->selected_mid_waypoint_index = -1;
							}
						}
					}
				}
				
				if(editor->selected_mid_waypoint_index >= 0)
				{
					V2 waypoint0 = storage->waypoints[editor->selected_mid_waypoint_index];
					push_line(render_info, waypoint0, p, 2.0f * DEFAULT_AGENT_RADIUS,
							  color_cyan * v4(1, 1, 1, 0.5f), rendering_priority_sim_overlay);
				}
				else
				{
					f32 radius = (target ? 0.80f : 0.70f) * DEFAULT_AGENT_RADIUS;
					f32 thickness = (target ? 0.20f : 0.15f) * DEFAULT_AGENT_RADIUS;
					V4 color = (target ? color_red : color_yellow) * v4(1, 1, 1, 0.5f);
					push_disk(render_info, p, radius, thickness, color, rendering_priority_sim_overlay);
				}
				break;
			}
		
			default:
			{
				crash();
			}
		}
	}

	u16 ui_rendering_priority_base = rendering_priority_ui_base;
	
	// professional quality ui panel
	push_rect(render_info, panel_rect, panel_color, ui_rendering_priority_base);

	// button 0
	push_circle(render_info, button0_p, button_r, button0_color, ui_rendering_priority_base + 1);
	push_circle(render_info, button0_p, circle_icon_r, button0_icon_color, ui_rendering_priority_base + 2);
	push_disk(render_info, button0_p, button_r + button_outline_thickness, button_outline_thickness, button0_outline_color, ui_rendering_priority_base + 2);

	// button 1
	push_circle(render_info, button1_p, button_r, button1_color, ui_rendering_priority_base + 1);
	push_line(render_info, button1_p - v2(line_icon_r, line_icon_r), button1_p + v2(line_icon_r, line_icon_r), line_icon_thickness, button1_icon_color, ui_rendering_priority_base + 2);
	push_disk(render_info, button1_p, button_r + button_outline_thickness, button_outline_thickness, button1_outline_color, ui_rendering_priority_base + 2);

	// button 2
	push_circle(render_info, button2_p, button_r, button2_color, ui_rendering_priority_base + 1);
	push_disk(render_info, button2_p, 0.8f * circle_icon_r, 0.1f * circle_icon_r, button2_icon_color, ui_rendering_priority_base + 2);
	push_disk(render_info, button2_p, button_r + button_outline_thickness, button_outline_thickness, button2_outline_color, ui_rendering_priority_base + 2);
	
	Rect2 sim_camera_rect = get_camera_rect(&storage->state.sim_camera);
	push_rect_outline(render_info, sim_camera_rect, debug_line_thickness, color_blue, 99);
}
