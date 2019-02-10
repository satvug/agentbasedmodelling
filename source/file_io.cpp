bool eat_whitespace(Tokenizer *tokenizer)
{
	char *cursor = tokenizer->string.data + tokenizer->offset;
	char *cursor_new = eat_whitespace(cursor);
	bool result = cursor != cursor_new;
    if(result)
	{
		tokenizer->offset += (u32)(cursor_new - cursor);
	}
	return result;
}

bool eat_comment(Tokenizer *tokenizer)
{
	bool result = tokenizer->string.data[tokenizer->offset] == '#';
	if(result)
	{
		while(tokenizer->offset < tokenizer->string.length && tokenizer->string.data[tokenizer->offset++] != '\n');
	}
	return result;
}

Token get_token(Tokenizer *tokenizer)
{
	Token result = {};
	result.type = token_type_unknown;

	while(eat_whitespace(tokenizer) || eat_comment(tokenizer));
	
	if(tokenizer->offset < tokenizer->string.length)
	{
		if(is_number(tokenizer->string.data[tokenizer->offset]))
		{
			result.type = token_type_number;
			result.string.data = tokenizer->string.data + tokenizer->offset;
			while(is_number(tokenizer->string.data[tokenizer->offset]))
			{
				++tokenizer->offset;
				++result.string.length;
			}
		}
		else if(is_alphabetic(tokenizer->string.data[tokenizer->offset]))
		{
			result.type = token_type_specifier;
			result.string.data = tokenizer->string.data + tokenizer->offset;
			while(is_alphabetic(tokenizer->string.data[tokenizer->offset]))
			{
				++tokenizer->offset;
				++result.string.length;
			}
		}
	}
	else
	{
		result.type = token_type_end_of_string;
	}
	
	return result;
}

Specifier get_specifier(String string)
{
	Specifier result = specifier_none;

    if(equal(string, const_string("sim_boundary")))
	{
		result = specifier_sim_boundary;
	}
    if(equal(string, const_string("obstacle_lines")) || equal(string, const_string("obstacles")))
	{
		result = specifier_obstacle_lines;
	}
    if(equal(string, const_string("obstacle_circles")))
	{
		result = specifier_obstacle_circles;
	}
    if(equal(string, const_string("obstacle_rects")))
	{
		result = specifier_obstacle_rects;
	}
	else if(equal(string, const_string("targets")))
	{
		result = specifier_targets;
	}
	else if(equal(string, const_string("sink_areas")))
	{
		result = specifier_sink_areas;
	}
	else if(equal(string, const_string("waypoints")))
	{
		result = specifier_waypoints;
	}
	else if(equal(string, const_string("agents")))
	{
		result = specifier_agents;
	}
	else if(equal(string, const_string("agent_positions")))
	{
		result = specifier_agent_positions;
	}
	else if(equal(string, const_string("paths")))
	{
		result = specifier_paths;
	}

	return result;
}

StatusCode load_environment(SimStorage *storage, String path)
{
	StatusCode result = status_code_none;

	u32 custom_path_count = 0;
	Path custom_paths[MAX_TARGET_WAYPOINT_COUNT * MAX_MID_WAYPOINT_COUNT];
	
	File input = read_entire_file(path);
	if(input.size > 0)
	{
		result = status_code_success;

		flush_storage(storage);
		Rect2 sim_boundary = center_dim_rect(v2(0, 0), v2(DEFAULT_ASPECT_RATIO * DEFAULT_VIEW_HEIGHT, DEFAULT_VIEW_HEIGHT));
		Tokenizer tokenizer;
		tokenizer.string = string(input);
		tokenizer.offset = 0;

		Token token = get_token(&tokenizer);
		if(token.type != token_type_specifier)
		{
			result = status_code_unknown_specifier;
		}
		while(token.type == token_type_specifier)
		{
			begin_tmp_memory(&storage->scratch_memory);
			f32 *current_specifier_values = (f32 *)get_remaining_memory(&storage->scratch_memory, sizeof(f32), memory_flag_none);
			Specifier current_specifier = get_specifier(token.string);
			u32 current_specifier_value_count = 0;
			if(current_specifier != specifier_none)
			{
				bool parsing_specifier = true;
				while(parsing_specifier)
				{
					Token value = get_token(&tokenizer);
				    if(value.type == token_type_number)
					{
						f32 valuef;
						if(string_to_float(value.string, &valuef))
						{
							current_specifier_values[current_specifier_value_count++] = valuef;
						}
					}
					else
					{
						u32 item_count = current_specifier_value_count >> 1;
					    V2 *values = (V2 *)current_specifier_values;
						switch(current_specifier)
						{
							case specifier_sim_boundary:
							{
								// TODO: if there is <= 2 values in the file, we can consider them width and height
								if(current_specifier_value_count == 4)
								{
								    sim_boundary.min = values[0];
								    sim_boundary.max = values[1];
								}
								else if(current_specifier_value_count == 2)
								{
								    sim_boundary = center_dim_rect(v2(0, 0), values[0]);
								}
								break;
							}
							case specifier_obstacle_lines:
							{
								for(u32 i = 0; i < item_count; i += 2)
								{
									add_obstacle_line(storage, values[i], values[i + 1]);
								}
								break;
							}
							case specifier_obstacle_circles:
							{
								for(u32 i = 0; i < current_specifier_value_count; i += 3)
								{
									V2 p = v2(current_specifier_values[i], current_specifier_values[i + 1]);
									f32 r = current_specifier_values[i + 2];
									add_obstacle_circle(storage, p, r);
								}
								break;
							}
							case specifier_obstacle_rects:
							{
								for(u32 i = 0; i < item_count; i += 2)
								{
									add_obstacle_rect(storage, values[i], values[i + 1]);
								}
								break;
							}
							case specifier_targets:
							{
								for(u32 i = 0; i < item_count; ++i)
								{
									add_waypoint(storage, values[i], true);
								}
								break;
							}
							case specifier_sink_areas:
							{
								for(u32 i = 0;
									i < item_count && storage->sink_rect_count < MAX_SINK_RECT_COUNT;
									i += 2)
								{
									Rect2 sink_rect = {values[i], values[i + 1]};
									storage->sink_rects[storage->sink_rect_count++] = sink_rect;
								}
								break;
							}
							case specifier_waypoints:
							{
								for(u32 i = 0; i < item_count; ++i)
								{
									add_waypoint(storage, values[i], false);
								}
								break;
							}
							case specifier_agent_positions:
							// NOTE: _potential_ agent placement.
							{
								for(u32 i = 0; i < item_count && storage->available_position_count < MAX_AGENT_COUNT; ++i)
								{
									storage->available_positions[storage->available_position_count++] = values[i];
								}
								break;
							}
							case specifier_agents:
							{
								for(u32 i = 0; i < item_count; ++i)
								{
									Agent *agent = default_agent(storage);
									place_agent(storage, agent, values[i]);
								}
								// TODO: get it working again just in case
								break;
							}
							case specifier_paths:
								// NOTE: target index, waypoint index from, waypoint index to
							{
								for(u32 i = 0; i < current_specifier_value_count; i += 3)
								{
									Path *custom_path = custom_paths + custom_path_count++;
									custom_path->target = (s16)current_specifier_values[i];
									custom_path->index_from = (s16)current_specifier_values[i + 1];
									custom_path->index_to = (s16)current_specifier_values[i + 2];
								}
								break;
							}
							default:
							{
								crash();
							}
						};
						token = value;
						parsing_specifier = false;
					}
				}
			}
			else
			{
				result = status_code_unknown_specifier;
				break;
			}
		}

		set_sim_boundary(storage, sim_boundary);
		free_file(&input);
	}
	else
	{
		result = status_code_invalid_path;
	}
	
	update_environment_graph(storage);
	s16 waypoint_count = storage->target_waypoint_count + storage->intermediate_waypoint_count;
	for(u32 i = 0; i < custom_path_count; ++i)
	{
		s16 target = custom_paths[i].target;
		s16 mid_from = custom_paths[i].index_from;
		s16 mid_to = custom_paths[i].index_to;
		if(target < storage->target_waypoint_count && mid_from < waypoint_count && mid_to < waypoint_count)
		{
			storage->shortest_path_indices[target][mid_from] = mid_to;
		}
	}
	storage->state.paused = true;
	
	return result;
}

void reload_environment(SimStorage *storage)
{
	load_environment(storage, storage->last_successful_load_path);
}

StatusCode save_environment(SimStorage *storage, String path)
{
	begin_tmp_memory(&storage->scratch_memory);

	u32 max_length = (u32)bytes_available(&storage->scratch_memory);
	String output_buffer;
	output_buffer.length = 0;
	output_buffer.data = push_array(&storage->scratch_memory, char, output_buffer.length);

	Rect2 boundary = storage->state.sim_boundary;
	append_formatted(&output_buffer, max_length, "sim_boundary\n%f %f %f %f\n",
					 boundary.min.x, boundary.min.y, boundary.max.x, boundary.max.y);

	append(&output_buffer, "agents\n", max_length);
	for(u32 i = 0; i < storage->agent_count; ++i)
	{
		Agent *agent = storage->agents + i;
		append_formatted(&output_buffer, max_length, "%f %f\n", agent->p.x, agent->p.y);
	}

	ObstacleType last_type = obstacle_type_none;
	for(u32 i = 0; i < storage->obstacle_count; ++i)
	{
		Obstacle *obstacle = storage->obstacles + i;
		switch(obstacle->type)
		{
			case obstacle_type_line:
			{
				if(last_type != obstacle_type_line)
				{
					append(&output_buffer, "obstacle_lines\n", max_length);
				}
				append_formatted(&output_buffer, max_length, "%f %f %f %f\n", obstacle->line.p0.x,
								 obstacle->line.p0.y, obstacle->line.p1.x, obstacle->line.p1.y);
				break;
			}
			case obstacle_type_circle:
			{
				if(last_type != obstacle_type_circle)
				{
					append(&output_buffer, "obstacle_circles\n", max_length);
				}
				append_formatted(&output_buffer, max_length, "%f %f %f\n", obstacle->circle.p.x,
								 obstacle->circle.p.y, obstacle->circle.r);
				break;
			}
			case obstacle_type_rect:
			{
				if(last_type != obstacle_type_rect)
				{
					append(&output_buffer, "obstacle_rects\n", max_length);
				}
				append_formatted(&output_buffer, max_length, "%f %f %f %f\n", obstacle->rect.min.x,
								 obstacle->rect.min.y, obstacle->rect.max.x, obstacle->rect.max.y);
				break;
			}
			default:
			{
				break;
			}
		}
		last_type = obstacle->type;
	}
	
	append(&output_buffer, "targets\n", max_length);
	for(u32 i = 0; i < storage->target_waypoint_count; ++i)
	{
		V2 target = storage->waypoints[i];
		append_formatted(&output_buffer, max_length, "%f %f\n", target.x, target.y);
	}

	append(&output_buffer, "waypoints\n", max_length);
	for(u32 i = 0; i < storage->intermediate_waypoint_count; ++i)
	{
		V2 waypoint = storage->waypoints[storage->target_waypoint_count + i];
		append_formatted(&output_buffer, max_length, "%f %f\n", waypoint.x, waypoint.y);
	}

	append(&output_buffer, "sink_areas\n", max_length);
	for(u32 i = 0; i < storage->sink_rect_count; ++i)
	{
		Rect2 rect = storage->sink_rects[i];
		append_formatted(&output_buffer, max_length, "%f %f %f %f\n", rect.min.x, rect.min.y, rect.max.x, rect.max.y);
	}
	
	append(&output_buffer, "agent_positions\n", max_length);
	for(u32 i = 0; i < storage->available_position_count; ++i)
	{
		V2 p = storage->available_positions[i];
		append_formatted(&output_buffer, max_length, "%f %f\n", p.x, p.y);
	}

	// NOTE: basically a definition of "wasteful"
	s16 waypoint_count = storage->target_waypoint_count + storage->intermediate_waypoint_count;
	append(&output_buffer, "paths\n", max_length);
	for(s32 i = 0; i < storage->target_waypoint_count; ++i)
	{
		s16 *shortest_paths = storage->shortest_path_indices[i];
		for(s32 j = storage->target_waypoint_count; j < waypoint_count; ++j)
		{
			append_formatted(&output_buffer, max_length, "%d %d %d\n", i, j, shortest_paths[j]);
		}
	}
	
	bool success = write_file(output_buffer.data, output_buffer.length, path);
	StatusCode result = success ? status_code_success : status_code_invalid_path;
	
	return result;
}

StatusCode dump_stats(SimStorage *storage, String path)
{
	begin_tmp_memory(&storage->scratch_memory);

	u32 max_length = (u32)bytes_available(&storage->scratch_memory);
	String output_buffer;
	output_buffer.length = 0;
	output_buffer.data = push_array(&storage->scratch_memory, char, max_length);

#if 0
	for(u32 i = 0; i < storage->current_run; ++i)
	{
		append(&output_buffer, "RUN_INFO\n\n", max_length);

		RunStats *run = storage->stats + i;

		append_formatted(&output_buffer, max_length, "desired_velocity : %f\n", run->params.desired_velocity);
		append_formatted(&output_buffer, max_length, "relaxation_time : %f\n", run->params.relaxation_time);
		append_formatted(&output_buffer, max_length, "radius : %f\n", run->params.radius);
		append_formatted(&output_buffer, max_length, "private_zone_radius : %f\n", run->params.private_zone_radius);
		append_formatted(&output_buffer, max_length, "repulsion_coefficient : %f\n", run->params.repulsion_coefficient);
		append_formatted(&output_buffer, max_length, "falloff_length : %f\n", run->params.falloff_length);
		append_formatted(&output_buffer, max_length, "body_force_constant : %f\n", run->params.body_force_constant);
		append_formatted(&output_buffer, max_length, "friction_coefficient : %f\n", run->params.friction_coefficient);
		append_formatted(&output_buffer, max_length, "random_force : %f\n", run->params.random_force);
		append_formatted(&output_buffer, max_length, "waypoint_update_distance : %f\n", run->params.waypoint_update_distance);
		append_formatted(&output_buffer, max_length, "waypoint_update_interval : %f\n", run->params.waypoint_update_interval);
		append_formatted(&output_buffer, max_length, "collision_avoidance : %f\n", run->params.collision_avoidance);
		append_formatted(&output_buffer, max_length, "agent_count : %d\n", run->agent_count);

		append(&output_buffer, '\n', max_length);
		
		append_formatted(&output_buffer, max_length, "evacuation_time : %f\n\n\n", run->run_time);
		//append_formatted(&output_buffer, max_length, "simulation_time : %f\n\n\n", run->run_time);
		
		//for(u32 j = 0; j < run->last_agent_index; ++j)
		//{
		//	AgentStats *agent = run->agent_records + j;
		//	AgentFrameNode *node = agent->front_sentinel.next;
		//	while(node->next)
		//	{
		//	    node = node->next;
		//	}
		//}
	}
#else
	// csv output
	char delimiter = ',';
	append_formatted(&output_buffer, max_length, "%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s\n",
					 "desired_velocity", delimiter, "relaxation_time", delimiter,
					 "radius", delimiter, "private_zone_radius", delimiter,
					 "repulsion_coefficient", delimiter, "falloff_length", delimiter,
					 "body_force_constant", delimiter, "friction_coefficient", delimiter,
					 "random_force", delimiter, "waypoint_update_distance", delimiter,
					 "waypoint_update_interval", delimiter, "collision_avoidance", delimiter,
					 "agent_count", delimiter, "evacuation_time");
	for(u32 i = 0; i < storage->current_run; ++i)
	{
		RunStats *run = storage->stats + i;
		append_formatted(&output_buffer, max_length, "%f%c%f%c%f%c%f%c%f%c%f%c%f%c%f%c%f%c%f%c%f%c%f%c%u%c%f\n",
						 run->params.desired_velocity, delimiter, run->params.relaxation_time, delimiter,
						 run->params.radius, delimiter, run->params.private_zone_radius, delimiter,
						 run->params.repulsion_coefficient, delimiter, run->params.falloff_length, delimiter,
						 run->params.body_force_constant, delimiter, run->params.friction_coefficient, delimiter,
						 run->params.random_force, delimiter, run->params.waypoint_update_distance, delimiter,
						 run->params.waypoint_update_interval, delimiter, run->params.collision_avoidance, delimiter,
						 run->agent_count, delimiter, run->run_time);
	}
#endif
	
	bool success = write_file(output_buffer.data, output_buffer.length, path);
	StatusCode result = success ? status_code_success : status_code_invalid_path;

	return result;
}
