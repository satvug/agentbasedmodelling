
void flush_storage(SimStorage *storage)
{
	storage->available_position_count = 0;
	storage->agent_count = 0;
	storage->obstacle_count = 0;
	storage->intermediate_waypoint_count = 0;
	storage->target_waypoint_count = 0;
	storage->sink_rect_count = 0;
	storage->selected_agent.value = 0;
	storage->selected_target_index = -1;
	while(is_valid(storage->last_agent_id))
	{
		delete_from_hash(storage, storage->last_agent_id);
		--storage->last_agent_id.value;
	}
	for(u32 i = 0; i < AGENT_ID_HASH_SIZE; ++i)
	{
		AgentIdHashNode *node = storage->agent_id_hash_table + i;
		node->id.value = 0;
		node->prev = node->next = node;
	}
	flush_partitions(storage);
}

void load_font(MemoryBlock *memory, RenderInfo *info, char *path)
{
	begin_tmp_memory(memory);
	
	File font_input = read_entire_file(path);
	u8 *input_buffer = (u8 *)font_input.data;
	FontFooter *footer = (FontFooter *)(input_buffer + font_input.size - sizeof(FontFooter));

	u8 *font_offset = input_buffer + footer->font_offset;
	Font *font = (Font *)font_offset;
	info->assets.debug_font = *font;

	u8 *input_atlas = font_offset + sizeof(Font);
	u32 atlas_resolution = info->assets.debug_font.atlas_resolution;
	u32 pixel_count = squared(atlas_resolution);
	u32 *transfer_atlas = push_array(memory, u32, pixel_count);
	for(u32 i = 0; i < pixel_count; ++i)
	{
		u32 alpha = (u32)(input_atlas[i]);
		u32 color = ~ALPHA_MASK | (alpha << ALPHA_SHIFT);
		transfer_atlas[i] = color;
	}
	
	u8 texture_handle_index = info->assets.texture_count++;
	for(s32 c1 = info->assets.debug_font.first_character; c1 <= info->assets.debug_font.last_character; ++c1)
	{
		FontGlyph *glyph = info->assets.debug_font.glyphs + c1 - info->assets.debug_font.first_character;		
		glyph->texture_handle_index = texture_handle_index;
	}

	glGenTextures(1, info->assets.texture_handles + texture_handle_index);
	glBindTexture(GL_TEXTURE_2D, info->assets.texture_handles[texture_handle_index]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas_resolution, atlas_resolution, 0, GL_RGBA, GL_UNSIGNED_BYTE, transfer_atlas);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	free_file(&font_input);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// NOTE: a routine to set up experiments
// probably we are in the "free run" mode when the sim's current run index is -1,
// and if we have a valid run index, we execute this run. How does it sound?
//
// for levels/classroom_5.abm
//                          |    min     |  max      | default
// agent_count              |    1.0000  |  100.0000 |   60.0000
// desired_velocity         |    1.0000  |    6.0000 |    1.5000
// relaxation_time          |    0.0500  |    1.0500 |    0.1500
// radius                   |    0.1000  |    0.3500 |    0.2250
// private_zone_radius      |    0.2000  |    0.5000 |    0.3200
// repulsion_coefficient    |    0.0000  |  100.0000 |    5.0000
// falloff_length           |    0.0500  |   10.0000 |    0.4000
// body_force_constant      |    0.0000  | 2000.0000 |  900.0000
// friction_coefficient     |    0.0000  | 1000.0000 |  400.0000
// random_force             |    0.0000  |   10.0000 |    0.1000
// waypoint_update_distance |    0.0000  |    1.0000 |    0.5000
// waypoint_update_interval |    0.3300  |    2.0000 |    1.0000
// collision_avoidance      |    0.0000  |    1.0000 |    0.2500
//
void init_runs(SimStorage *storage)
{
	u32 run_count = 100000;
	u32 agent_count_min = 1;
	u32 agent_count_max = 100;
	f32 desired_velocity_min = 1.0f;
	f32 desired_velocity_max = 6.0f;
	f32 relaxation_time_min = 0.05f;
	f32 relaxation_time_max = 1.50f;
	f32 radius_min = 0.15f;
	f32 radius_max = 0.30f;
	f32 private_zone_radius_min = 0.20f;
	f32 private_zone_radius_max = 0.37f;
	f32 repulsion_coefficient_min = 0.0f;
	f32 repulsion_coefficient_max = 100.0f;
	f32 falloff_length_min = 0.10f;
	f32 falloff_length_max = 10.0f;
	f32 body_force_constant_min = 0.00f;
	f32 body_force_constant_max = 2000.0f;
	f32 friction_coefficient_min = 0.00f;
	f32 friction_coefficient_max = 1000.0f;
	f32 random_force_min = 0.00f;
	f32 random_force_max = 10.0f;
	f32 waypoint_update_distance_min = 0.00f;
	f32 waypoint_update_distance_max = 1.00f;
	f32 waypoint_update_interval_min = 0.33f;
	f32 waypoint_update_interval_max = 2.00f;
	f32 collision_avoidance_min = 0.00f;
	f32 collision_avoidance_max = 1.00f;
	for(u32 i = 0; i < run_count; ++i)
	{
		u32 agent_count = random_between_i(&storage->rng, agent_count_min, agent_count_max);
		f32 desired_velocity = random_between_f(&storage->rng, desired_velocity_min, desired_velocity_max);
		f32 relaxation_time = random_between_f(&storage->rng, relaxation_time_min, relaxation_time_max);
		f32 radius = random_between_f(&storage->rng, radius_min, radius_max);
		f32 private_zone_radius = random_between_f(&storage->rng, private_zone_radius_min, private_zone_radius_max);
		f32 repulsion_coefficient = random_between_f(&storage->rng, repulsion_coefficient_min, repulsion_coefficient_max);
		f32 falloff_length = random_between_f(&storage->rng, falloff_length_min, falloff_length_max);
		f32 body_force_constant = random_between_f(&storage->rng, body_force_constant_min, body_force_constant_max);
		f32 friction_coefficient = random_between_f(&storage->rng, friction_coefficient_min, friction_coefficient_max);
		f32 random_force = random_between_f(&storage->rng, random_force_min, random_force_max);
		f32 waypoint_update_distance = random_between_f(&storage->rng, waypoint_update_distance_min, waypoint_update_distance_max);
		f32 waypoint_update_interval = random_between_f(&storage->rng, waypoint_update_interval_min, waypoint_update_interval_max);
		f32 collision_avoidance = random_between_f(&storage->rng, collision_avoidance_min, collision_avoidance_max);
		add_run(storage,
			    agent_count,
				1,
			    desired_velocity,
				relaxation_time,
				radius,
				private_zone_radius,
				repulsion_coefficient,
				falloff_length,
				body_force_constant,
				friction_coefficient,
				random_force,
				waypoint_update_distance,
				waypoint_update_interval,
				collision_avoidance);
	}
	/*
	u32 repeat_count = 100;
    for(u32 agent_count = 1; agent_count <= 100; ++agent_count)
	{
		add_run(storage, agent_count, repeat_count);
	}
	*/
	/*
	u32 default_agent_count = 100;
	u32 step_count = 100;
	u32 repeat_count = 100;
	
	f32 desired_velocity_min = 1.0f;
	f32 desired_velocity_max = 6.0f;
	for(u32 i = 0; i < step_count; ++i)
	{
		f32 t = i / (f32)(step_count - 1);
		f32 desired_velocity = lerp(desired_velocity_min, t, desired_velocity_max);
		add_run(storage,
			    default_agent_count,
				repeat_count,
			    desired_velocity,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	for(f32 relaxation_time = 0.05f; relaxation_time <= 1.04f; relaxation_time += 0.01f)
	{
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				//DEFAULT_AGENT_RELAXATION_TIME,
				relaxation_time,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 agent_radius_min = 0.1f;
	f32 agent_radius_max = 0.35f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 agent_radius = lerp(agent_radius_min, t, agent_radius_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				agent_radius,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 private_zone_radius_min = 0.2f;
	f32 private_zone_radius_max = 0.5f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 private_zone_radius = lerp(private_zone_radius_min, t, private_zone_radius_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				//DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				private_zone_radius,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 repulsion_coefficient_min = 0.0f;
	f32 repulsion_coefficient_max = 100.0f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 repulsion_coefficient = lerp(repulsion_coefficient_min, t, repulsion_coefficient_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				//DEFAULT_AGENT_REPULSION_COEFFICIENT,
			    repulsion_coefficient,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 falloff_length_min = 0.05f;
	f32 falloff_length_max = 10.0f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 falloff_length = lerp(falloff_length_min, t, falloff_length_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				//DEFAULT_AGENT_FALLOFF_LENGTH,
			    falloff_length,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 body_force_constant_min = 0.00f;
	f32 body_force_constant_max = 2000.0f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 body_force_constant = lerp(body_force_constant_min, t, body_force_constant_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
			    body_force_constant,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 friction_coefficient_min = 0.00f;
	f32 friction_coefficient_max = 1000.0f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 friction_coefficient = lerp(friction_coefficient_min, t, friction_coefficient_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
			    friction_coefficient,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 random_force_min = 0.00f;
	f32 random_force_max = 10.0f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 random_force = lerp(random_force_min, t, random_force_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
			    random_force,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 waypoint_update_distance_min = 0.00f;
	f32 waypoint_update_distance_max = 1.00f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 waypoint_update_distance = lerp(waypoint_update_distance_min, t, waypoint_update_distance_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
			    waypoint_update_distance,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 waypoint_update_interval_min = 0.33f;
	f32 waypoint_update_interval_max = 2.00f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 waypoint_update_interval = lerp(waypoint_update_interval_min, t, waypoint_update_interval_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				waypoint_update_interval,
				DEFAULT_AGENT_COLLISION_AVOIDANCE);
	}
	*/
	/*
	f32 collision_avoidance_min = 0.00f;
	f32 collision_avoidance_max = 1.00f;
	for(u32 i = 0; i < 100; ++i)
	{
		f32 t = i / 99.0f;
		f32 collision_avoidance = lerp(collision_avoidance_min, t, collision_avoidance_max);
		add_run(storage,
				DEFAULT_AGENT_COUNT,
				repeat_count,
			    DEFAULT_AGENT_DESIRED_VELOCITY,
				DEFAULT_AGENT_RELAXATION_TIME,
				DEFAULT_AGENT_RADIUS,
				DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
				DEFAULT_AGENT_REPULSION_COEFFICIENT,
				DEFAULT_AGENT_FALLOFF_LENGTH,
				DEFAULT_AGENT_BODY_FORCE_CONSTANT,
				DEFAULT_AGENT_FRICTION_COEFFICIENT,
				DEFAULT_AGENT_RANDOM_FORCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
				DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
				collision_avoidance);
	}
	*/
}

void init(SimStorage *storage, RenderInfo *info, char *executable_path, u32 random_seed = 13)
{
	assert(power_of_two(AGENT_ID_HASH_SIZE));
	
	f32 _h = 0.4f;
	V4 console_color = v4(0.2f, 0.2f, 0.2f, 0.9f);
	f32 time_to_open = 0.3f;
	console_init(&storage->console, _h, console_color, time_to_open);
				 
	//load_font(&storage->scratch_memory, info, "output.font");
	load_font(&storage->scratch_memory, info, executable_path);
	
	// NOTE: MAX_AGENT_COUNT gets doubled because we use ping-pong buffers to simplify
	// the process of keeping the proper agent placement (the agents located in the same
	// spacial partition grid cell should go contiguously in memory)
    storage->available_position_count = 0;
    storage->available_positions = push_array(&storage->persistent_memory, V2, MAX_AGENT_COUNT);
    storage->agent_count = 0;
	storage->agents = push_array(&storage->persistent_memory, Agent, 2 * MAX_AGENT_COUNT);
	storage->obstacle_count = 0;
	storage->obstacles = push_array(&storage->persistent_memory, Obstacle, MAX_OBSTACLE_COUNT);	
	storage->agent_id_hash_table = push_array(&storage->persistent_memory, AgentIdHashNode, AGENT_ID_HASH_SIZE);
	storage->waypoints = push_array(&storage->persistent_memory, V2, MAX_WAYPOINT_COUNT);
	storage->sink_rects = push_array(&storage->persistent_memory, Rect2, MAX_SINK_RECT_COUNT);
    storage->shortest_path_indices = push_array(&storage->persistent_memory, s16 *, MAX_TARGET_WAYPOINT_COUNT);
	for(u32 i = 0; i < MAX_TARGET_WAYPOINT_COUNT; ++i)
	{
		storage->shortest_path_indices[i] = push_array(&storage->persistent_memory, s16, MAX_WAYPOINT_COUNT);
	}
	storage->selected_agent.value = 0;
	storage->selected_target_index = -1;

	set_default_sim_params(&storage->state);
	init_default_environment(storage);
	fit_sim_camera(&storage->state, info);
	
	storage->state.sim_camera.p = info->target_camera_p;
	storage->state.sim_camera.view_dim = info->target_camera_view_dim;

	storage->editor.debug_camera = storage->state.sim_camera;

	storage->stats = push_array(&storage->stats_memory, RunStats, MAX_RUN_STATS_COUNT);
	storage->current_run = 0;
	storage->stats[storage->current_run].params = storage->state.params;
	storage->stats[storage->current_run].run_time = 0;
	storage->stats[storage->current_run].agent_count = 0;

	// TODO: seed it with time or something
	storage->rng.state = random_seed;

	storage->last_successful_load_path.data = storage->last_successful_load_path_buffer;
	storage->last_successful_load_path.length = 0;

	storage->runs = push_array(&storage->persistent_memory, Run, MAX_RUN_COUNT);
	storage->run_count = 0;
	init_runs(storage);

	editor_init(storage);
	V4 clear_color = v4(0.3f, 0.3f, 0.3f, 1.0f);
	renderer_init(info, &storage->state.sim_camera, clear_color);
}

void switch_mode(SimStorage *storage, RenderInfo *render_info)
{
	switch(storage->mode)
	{
		case sim_mode_sim:
		{
			storage->mode = sim_mode_editor;
			storage->editor.debug_camera = storage->state.sim_camera;
			set_current_camera(render_info, &storage->editor.debug_camera);
			storage->editor.panel.target_openness = storage->editor.panel.default_openness;
			storage->editor.panel.current_openness = storage->editor.panel.min_openness;
			break;
		}
		case sim_mode_editor:
		{
			set_edit_mode(&storage->editor, edit_mode_none);
			storage->mode = sim_mode_sim;
			storage->state.sim_camera = storage->editor.debug_camera;
			storage->state.real_time_sim = storage->state.paused = true;
			set_current_camera(render_info, &storage->state.sim_camera);
			storage->editor.debug_camera = storage->state.sim_camera;
			break;
		}
		default:
		{
			crash();
		}
	}
}

bool finalize_update(SimStorage *storage)
{
	//
	// This is the place where the proper agent placement gets restored.
	// I do believe it's possible to do it in place, but after some thinking
	// (and writing the messiest loop the mankind witnessed),
	// I decided to go with the clearer system utilizing ping-pong buffers
	// (go one after another in memory) and one pass of radix sort (using
	// partition index as a key).
	//
	// TODO: do we even need the hash lookup system?
	// Given that agent deletion is absolutely free with the following loop (just don't copy the agent over
	// and edit the storage->agent_count appropriately!),
	// it's just an unnecessary overhead, since each time an agent is relocated
	// in memory we need to update the hash to keep it up-to-date
	//

	Agent *agents = storage->agents + (storage->frame_counter & 1 ? -MAX_AGENT_COUNT : MAX_AGENT_COUNT);
    s32 *indices = push_array(&storage->scratch_memory, s32, storage->agent_count);
	u16 offsets[array_count(storage->sim_grid)] = {};
	for(u32 i = 0; i < storage->agent_count; ++i)
	{
		Agent *agent = storage->agents + i;
		s32 partition_index = get_partition_index(storage, agent->p);
		for(u32 j = 0; j < storage->sink_rect_count && partition_index >= 0; ++j)
		{
			if(intersect(storage->sink_rects[j], agent->p))
			{
				partition_index = -1;
			}
		}
		if(partition_index >= 0)
		{
			indices[i] = partition_index;
			++offsets[partition_index];
		}
		else
		{
			indices[i] = partition_index;
		}
	}
	
	u16 cumulative_offset = 0;
	for(u32 i = 0; i < array_count(offsets); ++i)
	{
		storage->sim_grid[i].first_agent_index = cumulative_offset;
		storage->sim_grid[i].one_over_last_agent_index = cumulative_offset + offsets[i];
		offsets[i] = cumulative_offset;
		cumulative_offset = storage->sim_grid[i].one_over_last_agent_index;
	}

	u16 deleted_agent_count = 0;
	for(u32 i = 0; i < storage->agent_count; ++i)
	{
		Agent *agent = storage->agents + i;
		s32 offset_index = indices[i];
		if(offset_index >= 0)
		{
			u16 new_index = offsets[offset_index]++;
			agents[new_index] = *agent;
			update_hash(storage, agents[new_index].id, new_index);
		}
		else
		{
			//AgentStats *stats = storage->stats[storage->current_run].agent_records + agent->stats_index;
			//stats->lifetime = agent->lifetime;
			delete_from_hash(storage, agent->id);
			++deleted_agent_count;
		}
	}
	storage->agent_count -= deleted_agent_count;
	storage->agents = agents;

	++storage->frame_counter;

	bool result = deleted_agent_count > 0 && storage->agent_count == 0;
	
	return result;
}

void update(SimStorage *storage, RenderInfo *render_info, Input *input, f32 dt)
{	
	begin_tmp_memory(&storage->scratch_memory);

	Input flushed_input = {};
	flushed_input.last_pressed_key = input->last_pressed_key;

	ProfessionalQualityConsole *console = &storage->console;
	if(is_pressed(input, control) && just_pressed(input, o))
	{
		toggle_console(console);
	}
	if(console->open)
	{
		char new_char = (char)input->last_pressed_key;
		update_console(storage, render_info, console, new_char);
		input = &flushed_input;
	}
	update_professional_quality_ui_panel_openness(&console->panel, dt);
	render_console(console, render_info);

	if(is_pressed(input, control) && just_pressed(input, i))
	{
		storage->state.real_time_sim = !storage->state.real_time_sim;
	}
	if(just_pressed(input, w))
	{
		show_debug_overlay = !show_debug_overlay;
	}
	if(just_pressed(input, e))
	{
		switch_mode(storage, render_info);
	}

	switch(storage->mode)
	{
		case sim_mode_sim:
		{
			if(just_pressed(input, space))
			{
				storage->state.paused = !storage->state.paused;
			}

			bool sim_finished = false;
			if(storage->state.real_time_sim)
			{
				dt = clamp(MIN_FRAME_TIME, dt, MAX_FRAME_TIME);
				update_sim(storage, render_info, input, dt);
			    sim_finished = finalize_update(storage);
			}
			else
			{
				dt = DEFAULT_FRAME_TIME;
				u32 update_count = 50;
				while(!sim_finished && update_count--)
				{
					update_sim(storage, render_info, input, dt);
					sim_finished = finalize_update(storage);
				}
			}
			
			if(sim_finished)
			{
				storage->stats[storage->current_run].params = storage->state.params;
				storage->stats[storage->current_run].agent_count = storage->state.last_requested_spawn_count;
				storage->stats[storage->current_run].run_time = storage->state.simulation_time;
				++storage->current_run;
				if(storage->state.current_run_index != -1)
				{
					s32 new_run_index = storage->state.current_run_index;
					u32 current_repeat_count = ++storage->state.current_run_repeat_count;
					if(current_repeat_count >= storage->runs[storage->state.current_run_index].repeat_count)
					{
					    new_run_index = storage->state.current_run_index + 1;
						if(!(new_run_index < (s32)storage->run_count))
						{
							new_run_index = -1;
							String path = const_string("default_output.csv");
							dump_stats(storage, path);
						    set_default_sim_params(&storage->state);
						}
					}
					set_run_index(storage, new_run_index);
				}
			}
			
			break;
		}
		case sim_mode_editor:
		{
			update_editor(storage, render_info, input, dt);
			break;
		}
		default:
		{
			crash();
		}
	}

	//
	// Rendering
	//

	Rect2 screen_rect = get_camera_rect(render_info->current_camera);
	if(storage->mode == sim_mode_sim && storage->state.paused)
	{
		String text = const_string("Paused");
		V2 p = {0.5f * (screen_rect.max.x + screen_rect.min.x), screen_rect.max.y};
		f32 line_height = get_height(screen_rect) * 0.06f;
		V2 offset = {0.07f * line_height, -0.07f * line_height};
		Font *font = &render_info->assets.debug_font;
		p = get_cursor_p_for_alignment(font, text, p, line_height, horizontal_alignment_center,
									   vertical_alignment_top);
		push_string(render_info, text, p, line_height, color_white, rendering_priority_ui_base + 1);
		push_string(render_info, text, p + offset, line_height, color_black, rendering_priority_ui_base);
	}

	if(show_debug_overlay)
	{
		if(storage->mode == sim_mode_sim)
		{
			char buffer[32];
			String text = {buffer, 0};
			append_formatted(&text, sizeof(buffer), "run %d/%u", storage->state.current_run_index, storage->run_count);
			V2 p = screen_rect.max;
			f32 line_height = get_height(screen_rect) * 0.06f;
			V2 offset = {0.07f * line_height, -0.07f * line_height};
			Font *font = &render_info->assets.debug_font;
			p = get_cursor_p_for_alignment(font, text, p, line_height, horizontal_alignment_right,
										   vertical_alignment_top);
			push_string(render_info, text, p, line_height, color_white, rendering_priority_ui_base + 1);
			push_string(render_info, text, p + offset, line_height, color_black, rendering_priority_ui_base);
		}
		
		for(u32 i = 0; i < storage->target_waypoint_count; ++i)
		{
			V2 waypoint = storage->waypoints[i];
			push_disk(render_info, waypoint, 0.8f * DEFAULT_AGENT_RADIUS,
					  0.20f * DEFAULT_AGENT_RADIUS,
					  color_red, rendering_priority_sim_overlay);
		}
		for(u32 i = storage->target_waypoint_count; i < ((u32)storage->target_waypoint_count + (u32)storage->intermediate_waypoint_count); ++i)
		{
			V2 waypoint = storage->waypoints[i];
			push_disk(render_info, waypoint, 0.7f * DEFAULT_AGENT_RADIUS,
					  0.15f * DEFAULT_AGENT_RADIUS,
					  color_yellow, rendering_priority_sim_overlay);
		}
		V4 sink_color = v4(1, 0, 0, 0.5f);
		for(u32 i = 0; i < (u32)storage->sink_rect_count; ++i)
		{
			push_rect(render_info, storage->sink_rects[i],
					  sink_color, rendering_priority_sim_background);
		}
		for(u32 i = 0; i < storage->available_position_count; ++i)
		{
			V2 p = storage->available_positions[i];
			V4 color = color_cyan * v4(1, 1, 1, 0.5f);
			push_disk(render_info, p, DEFAULT_AGENT_RADIUS, DEBUG_LINE_THICKNESS, color, rendering_priority_sim_background);
		}
		Rect2 sim_boundary = storage->state.sim_boundary;
		sim_boundary.min -= DEBUG_LINE_THICKNESS * v2(0.5f, 0.5f);
		sim_boundary.max += DEBUG_LINE_THICKNESS * v2(0.5f, 0.5f);
		push_rect_outline(render_info, sim_boundary, DEBUG_LINE_THICKNESS,
						  color_green, rendering_priority_sim_overlay);
		if(storage->selected_target_index >= 0)
		{
			s16 *shortest_paths = storage->shortest_path_indices[storage->selected_target_index];
			for(u32 i = storage->target_waypoint_count; i < ((u32)storage->target_waypoint_count + (u32)storage->intermediate_waypoint_count); ++i)
			{
				V2 waypoint0 = storage->waypoints[i];
				s16 path_index = shortest_paths[i];
				if(path_index >= 0)
				{
					V2 waypoint1 = storage->waypoints[path_index];
					push_line(render_info, waypoint0, waypoint1, 2.0f * DEFAULT_AGENT_RADIUS,
							  color_cyan * v4(1, 1, 1, 0.5f), rendering_priority_sim_background);
				}
			}
		}
	}

	for(u32 obstacle_index = 0; obstacle_index < storage->obstacle_count; ++obstacle_index)
	{
		Obstacle *obstacle = storage->obstacles + obstacle_index;
		draw_obstacle(render_info, obstacle);
	}
	for(u32 agent_index = 0; agent_index < storage->agent_count; ++agent_index)
	{
		Agent *agent = storage->agents + agent_index;
		V4 color;
		if(agent->id == storage->selected_agent)
		{
			color = color_red;
			//AgentFrameNode *prev = storage->stats[storage->current_run].agent_records[agent->stats_index].front_sentinel.next;
			//AgentFrameNode *next = prev->next;
			//while(next)
			//{				
			//	push_line(render_info, prev->p, next->p, DEBUG_LINE_THICKNESS,
			//			  color_cyan, rendering_priority_sim_background);
			//	prev = next;
			//    next = next->next;
			//}
		}
		else
		{
			color = color_white;
		}
		u16 rendering_priority = (u16)(rendering_priority_sim_base + agent->id.value %
									   (rendering_priority_ui_base - rendering_priority_sim_base));
		push_circle(render_info, agent->p, agent->radius, color, rendering_priority_sim_base);
		if(show_debug_overlay)
		{
			f32 radius = agent->private_zone_radius;
			push_disk(render_info, agent->p, radius, 0.1f * agent->radius,
					  color_green, rendering_priority);
		}
	}
}
