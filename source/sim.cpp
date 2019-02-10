
//
// NOTE: Here goes the set of horribly slow functions that are _not_ supposed to be used in runtime
//

f32 get_waypoint_distance(SimStorage *storage, u32 index_a, u32 index_b)
{
	V2 a = storage->waypoints[index_a];
	V2 b = storage->waypoints[index_b];
	f32 distance = infinity();
	bool obstructed = false;
	for(u16 j = 0; j < storage->obstacle_count && !obstructed; ++j)
	{
		Obstacle *obstacle = storage->obstacles + j;
#if 0
		obstructed = intersect(obstacle->p0, obstacle->p1, a, b);
#else
		V2 line_n = DEFAULT_AGENT_RADIUS * normalize(perp(a - b));
		//obstructed = intersect(obstacle, a + line_n, b + line_n) || intersect(obstacle, a - line_n, b - line_n);
		obstructed =
			intersect(obstacle, a, b) &&
			(intersect(obstacle, a + line_n, b + line_n) ||
			 intersect(obstacle, a - line_n, b - line_n));
#endif
	}
	if(!obstructed)
	{
		distance = length(a - b);
	}
	return distance;
}

// Dijkstra's algorithm
// finds shortest paths between a target waypoint and all the non-target waypoints

void heap_insert(Heap *heap, f32 node_key, s16 node_index, s16 *heap_indices)
{
    s16 index = (s16)heap->size++;
    HeapEntry entry = {node_key, node_index};
    while(index > 0)
    {
        s16 parent = (index - 1) / 2;
        if(heap->entries[parent].key > node_key)
        {
            heap->entries[index] = heap->entries[parent];
            heap_indices[heap->entries[index].index] = index;
            
            heap->entries[parent] = entry;
            heap_indices[heap->entries[parent].index] = parent;
            
            index = parent;
        }
        else
        {
            break;
        }
    }
    heap->entries[index] = entry;
    heap_indices[heap->entries[index].index] = index;
}

s16 heap_remove(Heap *heap, s16 index, s16 *heap_indices)
{
    s16 result = -1;
    
    if(heap->size > index)
    {
        result = heap->entries[index].index;
        HeapEntry entry = heap->entries[--heap->size];
        heap->entries[index] = entry;

        while(index > 0)
        {
            s16 parent = (index - 1) / 2;
            if(heap->entries[parent].key > entry.key)
            {
                heap->entries[index] = heap->entries[parent];
                heap_indices[heap->entries[index].index] = index;
                
                heap->entries[parent] = entry;
                heap_indices[heap->entries[parent].index] = parent;
                
                index = parent;
            }
            else
            {
                break;
            }
        }
        heap->entries[index] = entry;
        heap_indices[heap->entries[index].index] = index;

        while(index < heap->size)
        {
            s16 child_0 = 2 * index + 1;
            s16 child_1 = 2 * index + 2;

            if(child_0 < heap->size)
            {
                if((child_1 < heap->size) &&
                   (heap->entries[child_1].key < heap->entries[child_0].key))
                {
                    child_0 = child_1;
                }
                
                if(entry.key > heap->entries[child_0].key)
                {
                    heap->entries[index] = heap->entries[child_0];
                    heap_indices[heap->entries[index].index] = index;
                    
                    heap->entries[child_0] = entry;
                    heap_indices[heap->entries[child_0].index] = child_0;
                    
                    index = child_0;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
	
    return result;
}

void update_shortest_paths(SimStorage *storage, s16 target_index = 0)
{
	begin_tmp_memory(&storage->scratch_memory);

	f32 inf = infinity();
	s16 waypoint_count = storage->target_waypoint_count + storage->intermediate_waypoint_count;
	
    Heap heap = {};
    heap.entries = push_array(&storage->scratch_memory, HeapEntry, waypoint_count);
	s16 *paths = storage->shortest_path_indices[target_index];
    s16 *heap_indices = push_array(&storage->scratch_memory, s16, waypoint_count);
    f32 *distances = push_array(&storage->scratch_memory, f32, waypoint_count);
    bool *visited = push_array(&storage->scratch_memory, bool, waypoint_count);
    for(s16 i = 0; i < waypoint_count; ++i)
    {
        distances[i] = inf;
        visited[i] = false;
        paths[i] = -1;
    }

	s16 current_index = target_index;
	distances[current_index] = 0;
	do
	{
		// NOTE: we dont' care about the shortest paths _between_ targets.
		// Having a target point on a path to another target point is a bug.
		for(s16 node_index = storage->target_waypoint_count; node_index < waypoint_count; ++node_index)
		{
			if(current_index != node_index && !visited[node_index])
			{
				f32 distance = distances[current_index] + get_waypoint_distance(storage, current_index, node_index);
				// NOTE: adding new points into the path decreases the distance a little bit so
				// the path having the same length but containing more nodes is preferable?
				distance -= epsilon;
				if(distance < inf)
				{
					if(distances[node_index] > distance)
					{
						if(distances[node_index] < inf)
						{
							s16 heap_index = heap_indices[node_index];
							heap_remove(&heap, heap_index, heap_indices);
						}
						distances[node_index] = distance;
						paths[node_index] = current_index;

						heap_insert(&heap, distance, node_index, heap_indices);
					}
				}
			}
		}
        visited[current_index] = true;
        s16 current_heap_index = heap_indices[current_index];
		current_index = heap_remove(&heap, current_heap_index, heap_indices);
	}
	while(current_index >= 0);
}

void update_environment_graph(SimStorage *storage)
{
	for(s16 i = 0; i < storage->target_waypoint_count; ++i)
	{
		update_shortest_paths(storage, i);
	}
}

void set_waypoint_position(SimStorage *storage, s16 index, V2 p)
{
	if(index >= 0)
	{
		storage->waypoints[index] = p;
		update_environment_graph(storage);
	}
}

void add_waypoint(SimStorage *storage, V2 p, bool target = false)
{
	s16 index = -1;
	if(target && storage->target_waypoint_count < MAX_TARGET_WAYPOINT_COUNT)
	{
		for(u32 i = storage->intermediate_waypoint_count; i > 0; --i)
		{
			storage->waypoints[storage->target_waypoint_count + i] =
				storage->waypoints[storage->target_waypoint_count + i - 1];
		}
		index = storage->target_waypoint_count++;
	}
	else if(storage->intermediate_waypoint_count < MAX_MID_WAYPOINT_COUNT)
	{
		index = storage->target_waypoint_count + storage->intermediate_waypoint_count++;
	}

	if(index >= 0)
	{
		storage->waypoints[index] = p;
	}
}

//
//
//

AgentParams default_agent_params()
{
	AgentParams result;
	result.desired_velocity = DEFAULT_AGENT_DESIRED_VELOCITY;
	result.relaxation_time = DEFAULT_AGENT_RELAXATION_TIME;
	result.radius = DEFAULT_AGENT_RADIUS;
	result.private_zone_radius = DEFAULT_AGENT_PRIVATE_ZONE_RADIUS;
	result.repulsion_coefficient = DEFAULT_AGENT_REPULSION_COEFFICIENT;
	result.falloff_length = DEFAULT_AGENT_FALLOFF_LENGTH;
	result.body_force_constant = DEFAULT_AGENT_BODY_FORCE_CONSTANT;
	result.friction_coefficient = DEFAULT_AGENT_FRICTION_COEFFICIENT;
	result.random_force = DEFAULT_AGENT_RANDOM_FORCE;
	result.waypoint_update_distance = DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE;
	result.waypoint_update_interval = DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL;
	result.collision_avoidance = DEFAULT_AGENT_COLLISION_AVOIDANCE;
	return result;
}

void add_run(SimStorage *storage,
			 u32 agent_count = DEFAULT_AGENT_COUNT,
			 u32 repeat_count = DEFAULT_REPEAT_COUNT,
			 f32 desired_velocity = DEFAULT_AGENT_DESIRED_VELOCITY,
			 f32 relaxation_time = DEFAULT_AGENT_RELAXATION_TIME,
			 f32 radius = DEFAULT_AGENT_RADIUS,
			 f32 private_zone_radius = DEFAULT_AGENT_PRIVATE_ZONE_RADIUS,
			 f32 repulsion_coefficient = DEFAULT_AGENT_REPULSION_COEFFICIENT,
			 f32 falloff_length = DEFAULT_AGENT_FALLOFF_LENGTH,
			 f32 body_force_constant = DEFAULT_AGENT_BODY_FORCE_CONSTANT,
			 f32 friction_coefficient = DEFAULT_AGENT_FRICTION_COEFFICIENT,
			 f32 random_force = DEFAULT_AGENT_RANDOM_FORCE,
			 f32 waypoint_update_distance = DEFAULT_AGENT_WAYPOINT_UPDATE_DISTANCE,
			 f32 waypoint_update_interval = DEFAULT_AGENT_WAYPOINT_UPDATE_INTERVAL,
			 f32 collision_avoidance = DEFAULT_AGENT_COLLISION_AVOIDANCE)
{
	Run *run = storage->runs + storage->run_count++;
	run->params.desired_velocity = desired_velocity;
	run->params.relaxation_time = relaxation_time;
	run->params.radius = radius;
	run->params.private_zone_radius = private_zone_radius;
	run->params.repulsion_coefficient = repulsion_coefficient;
	run->params.falloff_length = falloff_length;
	run->params.body_force_constant = body_force_constant;
	run->params.friction_coefficient = friction_coefficient;
	run->params.random_force = random_force;
	run->params.waypoint_update_distance = waypoint_update_distance;
	run->params.waypoint_update_interval = waypoint_update_interval;
	run->params.collision_avoidance = collision_avoidance;
	run->agent_count = agent_count;
	run->repeat_count = repeat_count;
}

void set_default_sim_params(SimState *state)
{
	state->paused = true;
	state->real_time_sim = true;

	state->current_run_index = -1;

	// TODO: remove
	state->remaining_reloads = MAX_RUN_STATS_COUNT;
	state->simulation_time = 0;
	state->last_requested_spawn_count = 0;
	state->params = default_agent_params();
}

void set_sim_boundary(SimStorage *storage, Rect2 boundary)
{
	SimState *state = &storage->state;
	state->sim_boundary = boundary;
	//assert(array_count(storage->sim_grid) < U8_MAX);
	V2 sim_dim = get_dim(state->sim_boundary);
	V2 cell_dim = sim_dim / (f32)SIM_GRID_DIM;
	for(s32 y = 0; y < SIM_GRID_DIM; ++y)
	{
		f32 y_offset = state->sim_boundary.min.y + y * cell_dim.y;
		for(s32 x = 0; x < SIM_GRID_DIM; ++x)
		{
			f32 x_offset = state->sim_boundary.min.x + x * cell_dim.x;
			s32 partition_index = y * SIM_GRID_DIM + x;
			WorldPartition *partition = storage->sim_grid + partition_index;
		    partition->boundary = rect(x_offset, y_offset, x_offset + cell_dim.x, y_offset + cell_dim.y);
		}
	}
	storage->sim_grid[array_count(storage->sim_grid) - 1].boundary = negative_infinity_rect();
}

void fit_sim_camera(SimState *state, RenderInfo *render_info)
{
	Rect2 sim_boundary = state->sim_boundary;
	V2 sim_dim = get_dim(sim_boundary);
	f32 aspect_ratio = DEFAULT_ASPECT_RATIO;
	f32 target_height = 1.1f * sim_dim.y;
	f32 target_width = aspect_ratio * target_height;
	if(target_width < 1.1f * sim_dim.x)
	{
		target_width = 1.1f * sim_dim.x;
		target_height = target_width / aspect_ratio;
	}
	set_target_camera_p(render_info, get_center(sim_boundary));
	set_view_dim(render_info, v2(target_width, target_height));
}

void init_default_environment(SimStorage *storage)
{
	flush_storage(storage);
	Rect2 sim_boundary = center_dim_rect(v2(0, 0), v2(DEFAULT_ASPECT_RATIO * DEFAULT_VIEW_HEIGHT, DEFAULT_VIEW_HEIGHT));
	set_sim_boundary(storage, sim_boundary);
	update_environment_graph(storage);
	storage->state.paused = true;
}

void set_run_index(SimStorage *storage, s32 run_index)
{
	s32 current_run_index = storage->state.current_run_index;
	storage->state.current_run_index = run_index;
	if(run_index > -1)
	{
		storage->state.params = storage->runs[run_index].params;
		storage->state.last_requested_spawn_count = storage->runs[run_index].agent_count;
		storage->state.simulation_time = 0;
		spawn_random_agents(storage, storage->state.last_requested_spawn_count);
		storage->state.simulation_time = 0;
		if(current_run_index != run_index)
		{
			storage->state.current_run_repeat_count = 0;
		}
	}
}

void update_current_waypoint(SimStorage *storage, Agent *agent)
{
	f32 inf = infinity();
	V2 p = agent->p;

	begin_tmp_memory(&storage->scratch_memory);

	Waypoint result = {p, visibility_none};

	s16 waypoint_count = storage->target_waypoint_count + storage->intermediate_waypoint_count;
	f32 *distances_sq = push_array(&storage->scratch_memory, f32, waypoint_count, sizeof(f32), memory_flag_none);
	V2 *waypoints = push_array(&storage->scratch_memory, V2, waypoint_count, sizeof(V2), memory_flag_none);
	Visibility *visibilities = push_array(&storage->scratch_memory, Visibility, waypoint_count, sizeof(Visibility), memory_flag_none);

	// NOTE: iterate through all the targets and mid waypoints,
	// calculate distances, and sort targets by distance
	for(s16 i = 0; i < storage->target_waypoint_count; ++i)
	{
		f32 distance_sq = length_squared(storage->waypoints[i] - p);
		s16 insertion_index = i;
		for(s16 j = i - 1; j >= 0; --j)
		{
			if(distances_sq[j] > distance_sq)
			{
				distances_sq[insertion_index] = distances_sq[j];
				waypoints[insertion_index] = waypoints[j];
				visibilities[insertion_index] = visibilities[j];
				--insertion_index;
			}
			else
			{
				break;
			}
		}
		waypoints[insertion_index] = storage->waypoints[i];
		distances_sq[insertion_index] = distance_sq;
		visibilities[insertion_index] = visibility_direct;
	}
	for(s16 i = storage->target_waypoint_count; i < waypoint_count; ++i)
	{
		waypoints[i] = storage->waypoints[i];
		distances_sq[i] = length_squared(waypoints[i] - p);
		visibilities[i] = visibility_direct;
	}

	// NOTE: iterate through obstacles and detect obstructed waypoints.
	// Fill in the visibilities array
	
	s16 non_obstructed_waypoint_count = waypoint_count;
	s16 *non_obstructed_waypoint_indices_0 = push_array(&storage->scratch_memory, s16, waypoint_count, sizeof(s16), memory_flag_none);
	s16 *non_obstructed_waypoint_indices_1 = push_array(&storage->scratch_memory, s16, waypoint_count, sizeof(s16), memory_flag_none);
	for(s16 i = 0; i < waypoint_count; ++i)
	{
		non_obstructed_waypoint_indices_0[i] = i;
	}

	//u64 obstruction_test_startup = __rdtsc();
	//u64 intersection_test_cycles = 0;
	for(s16 i = 0; i < storage->obstacle_count &&
			non_obstructed_waypoint_count > 0; ++i)
	{
		Obstacle *obstacle = storage->obstacles + i;

		if(non_obstructed_waypoint_count > 0)
		{
			s16 index_count = 0;
			s16 *current_buffer, *next_buffer;
			if(i & 1)
			{
				current_buffer = non_obstructed_waypoint_indices_1;
				next_buffer = non_obstructed_waypoint_indices_0;
			}
			else
			{
				current_buffer = non_obstructed_waypoint_indices_0;
				next_buffer = non_obstructed_waypoint_indices_1;
			}
			
			for(s32 j = 0; j < non_obstructed_waypoint_count; ++j)
			{
				s16 index = current_buffer[j];
				
				V2 waypoint = waypoints[index];
				// NOTE: normal points to the _left_ (CCW) relative to the line
				f32 line_n_x = p.y - waypoint.y;
				f32 line_n_y = waypoint.x - p.x;
				f32 length = sqrt(line_n_x * line_n_x + line_n_y * line_n_y);
				//f32 scale = DEFAULT_AGENT_RADIUS / length;
				f32 scale = agent->radius / length;

				line_n_x *= scale;
				line_n_y *= scale;
				V2 line_n = {line_n_x, line_n_y};

				switch(visibilities[index])
				{
					case visibility_direct:
					{
						bool obstructed_left = intersect(obstacle, p + line_n, waypoint + line_n);
						bool obstructed_right = intersect(obstacle, p - line_n, waypoint - line_n);
						if(obstructed_left && obstructed_right)
						{
							distances_sq[index] = inf;
							visibilities[index] = visibility_none;
						}
						else
						{
							next_buffer[index_count++] = index;
							if(obstructed_left)
							{
								visibilities[index] = visibility_right;
							}
							else if(obstructed_right)
							{
								visibilities[index] = visibility_left;
							}
						}
						break;
					}
					case visibility_left:
					{
						bool obstructed_left = intersect(obstacle, p + line_n, waypoint + line_n);
						if(obstructed_left)
						{
							distances_sq[index] = inf;
							visibilities[index] = visibility_none;
						}
						else
						{
							next_buffer[index_count++] = index;
						}
						break;
					}
					case visibility_right:
					{
						bool obstructed_right = intersect(obstacle, p - line_n, waypoint - line_n);
						if(obstructed_right)
						{
							distances_sq[index] = inf;
							visibilities[index] = visibility_none;
						}
						else
						{
							next_buffer[index_count++] = index;
						}
						break;
					}
					default:
					{
						break;
					}
				};
			}
			non_obstructed_waypoint_count = index_count;
		}
	}
	//u64 obstruction_test_elapsed = __rdtsc() - obstruction_test_startup;

	s16 closest_intermediate_waypoint_index = -1;
	f32 closest_intermediate_waypoint_distance_sq = inf;
	for(s16 i = storage->target_waypoint_count; i < waypoint_count; ++i)
	{
		if(distances_sq[i] < closest_intermediate_waypoint_distance_sq)
		{
			closest_intermediate_waypoint_index = i;
			closest_intermediate_waypoint_distance_sq = distances_sq[i];
		}
	}

	for(s16 i = 0; i < storage->target_waypoint_count; ++i)
	{
		if(distances_sq[i] < inf)
		{
			result.p = waypoints[i];
			result.visibility = visibilities[i];
			break;
		}
		else if(closest_intermediate_waypoint_index >= 0)
		{
			s16 *shortest_paths = storage->shortest_path_indices[i];
			s16 path_index = shortest_paths[closest_intermediate_waypoint_index];
			if(path_index >= 0)
			{
				while(distances_sq[path_index] < inf)
				{
					closest_intermediate_waypoint_index = path_index;
					path_index = shortest_paths[path_index];
				}
				result.p = waypoints[closest_intermediate_waypoint_index];
				result.visibility = visibilities[closest_intermediate_waypoint_index];
				break;
			}
		}
	}

	//u64 elapsed = __rdtsc() - startup;
	
	agent->current_waypoint = result;
}

void update_sim(SimStorage *storage, RenderInfo *render_info, Input *input, f32 dt)
{
	SimState *state = &storage->state;

    if(state->paused || storage->agent_count == 0)
	{
		return;
	}
	else
	{
	    state->simulation_time += dt;
	}

	f32 radius = state->params.private_zone_radius + state->params.radius;
	Rect2 sim_boundary = state->sim_boundary;
	f32 w = get_width(sim_boundary);
	f32 h = get_height(sim_boundary);

	//
	// DEBUG PLACE
	V2 cursor_p = unproject(input->mouse_p, render_info);
	
	for(u16 i = 0; i < storage->agent_count; ++i)
	{
		Agent *agent = storage->agents + i;
		
		f32 waypoint_line_n_x = agent->p.y - agent->current_waypoint.p.y;
		f32 waypoint_line_n_y = agent->current_waypoint.p.x - agent->p.x;
		f32 waypoint_line_length = sqrt(waypoint_line_n_x * waypoint_line_n_x + waypoint_line_n_y * waypoint_line_n_y);
		f32 scale = agent->radius / waypoint_line_length;
		waypoint_line_n_x *= scale;
		waypoint_line_n_y *= scale;
		V2 waypoint_line_n = {waypoint_line_n_x, waypoint_line_n_y};
		if(agent->current_waypoint.visibility != visibility_none)
		{
			agent->current_waypoint.visibility = visibility_direct;
		}
		
		V2 force_interaction = v2();
		for(u16 j = 0; j < storage->obstacle_count; ++j)
		{
			Obstacle *obstacle = storage->obstacles + j;

			// NOTE: current waypoint visibility update
			switch(agent->current_waypoint.visibility)
			{
				case visibility_direct:
				{
					bool obstructed_left = intersect(obstacle, agent->p + waypoint_line_n,
													 agent->current_waypoint.p + waypoint_line_n);
					bool obstructed_right = intersect(obstacle, agent->p - waypoint_line_n,
													  agent->current_waypoint.p - waypoint_line_n);
					if(obstructed_left && obstructed_right)
					{
						agent->current_waypoint.visibility =
							visibility_none;
					}
					else
					{
						if(obstructed_left)
						{
							agent->current_waypoint.visibility =
								visibility_right;
						}
						else if(obstructed_right)
						{
							agent->current_waypoint.visibility =
								visibility_left;
						}
					}
					break;
				}
				case visibility_left:
				{
					if(intersect(obstacle, agent->p + waypoint_line_n,
								 agent->current_waypoint.p + waypoint_line_n))
					{
						agent->current_waypoint.visibility =
							visibility_none;
					}
					break;
				}
				case visibility_right:
				{
					if(intersect(obstacle, agent->p - waypoint_line_n,
								 agent->current_waypoint.p - waypoint_line_n))
					{
						agent->current_waypoint.visibility =
							visibility_none;
					}
					break;
				}
				default:
				{
					break;
				}
			};
			
			V2 v = agent->p - get_closest_point(obstacle, agent->p);
			f32 distance_sq = v.x * v.x + v.y * v.y;
			f32 r = agent->private_zone_radius + obstacle->thickness;
			if(distance_sq < r * r)
			{
				f32 distance = sqrt(distance_sq);
				V2 normal = v / distance;
				f32 overlap = agent->radius  + obstacle->thickness - distance;
				if(overlap > 0)
				{
					V2 tangent = perp(normal);
					f32 friction_coefficient = -0.5f * (agent->friction_coefficient + 
													   obstacle->friction_coefficient);
					f32 projection = dot(agent->dp, tangent);
					if(projection > 0)
					{
						// NOTE: shouldn't be necessary
					    tangent = -tangent;
					}
					V2 friction_force = friction_coefficient *
						dot(agent->dp, tangent) * overlap * tangent;
					
					f32 body_force_constant = 0.5f * (agent->body_force_constant +
													  obstacle->body_force_constant);
					V2 restoring_force = body_force_constant * overlap * normal;
					
					force_interaction += restoring_force + friction_force;
				}
				V2 repulsion_force = agent->repulsion_coefficient *
					exp(overlap / agent->falloff_length) * normal;
				force_interaction += repulsion_force;
			}
		}

		s32 min_x = (s32)((((agent->p.x - radius) - sim_boundary.min.x) / w) * SIM_GRID_DIM);
		if(min_x < 0) min_x = 0;
		s32 max_x = (s32)((((agent->p.x + radius) - sim_boundary.min.x) / w) * SIM_GRID_DIM);
		if(max_x > SIM_GRID_DIM - 1) max_x = SIM_GRID_DIM - 1;
		s32 min_y = (s32)((((agent->p.y - radius) - sim_boundary.min.y) / h) * SIM_GRID_DIM);
		if(min_y < 0) min_y = 0;
		s32 max_y = (s32)((((agent->p.y + radius) - sim_boundary.min.y) / h) * SIM_GRID_DIM);
		if(max_y > SIM_GRID_DIM - 1) max_y = SIM_GRID_DIM - 1;
		for(s32 y = min_y; y <= max_y; ++y)
		{
			for(s32 x = min_x; x <= max_x; ++x)
			{
				WorldPartition *partition = storage->sim_grid + y * SIM_GRID_DIM + x;
				if(intersect(partition->boundary, agent->p, radius))
				{
					for(u16 j = partition->first_agent_index; j < partition->one_over_last_agent_index; ++j)
					{
						if(j == i)
						{
							continue;
						}
						Agent *other_agent = storage->agents + j;
						V2 v = agent->p - other_agent->p;
						f32 distance_sq = v.x * v.x + v.y * v.y;
						f32 r = agent->private_zone_radius + other_agent->radius;
						if(distance_sq < r * r)
						{
							f32 distance = sqrt(distance_sq);
							V2 normal = v / distance;
							f32 overlap = agent->radius + other_agent->radius - distance;
							if(overlap > 0)
							{
								V2 tangent = perp(normal);
								f32 friction_coefficient = 0.5f * (agent->friction_coefficient +
																   other_agent->friction_coefficient);
								f32 projection = dot(agent->dp, tangent);
								if(projection > 0)
								{
								    tangent = -tangent;
								}
								V2 friction_force = -friction_coefficient *
									dot(agent->dp, tangent) * overlap * tangent;
								
								f32 body_force_constant = 0.5f * (agent->body_force_constant +
																  other_agent->body_force_constant);
								V2 restoring_force = body_force_constant * overlap * normal;

								force_interaction += restoring_force + friction_force;
							}
							V2 repulsion_force = agent->repulsion_coefficient *
								exp(overlap / agent->falloff_length) * normal;
							force_interaction += repulsion_force;
						}
					}
				}
			}
		}

		V2 random_force = state->params.random_force * v2(random_between_f(&storage->rng, -1, 1),
														  random_between_f(&storage->rng, -1, 1));

	    if(waypoint_line_length < state->params.waypoint_update_distance ||
		   agent->current_waypoint.visibility == visibility_none ||
		   (agent->lifetime - agent->last_waypoint_update >= state->params.waypoint_update_interval))
		{
			update_current_waypoint(storage, agent);
			agent->last_waypoint_update = agent->lifetime;
		}
		Waypoint closest_waypoint = agent->current_waypoint;
		V2 current_waypoint = closest_waypoint.p;		
		V2 target_direction = current_waypoint - agent->p;
		f32 collision_avoidance = state->params.collision_avoidance;
	    if(closest_waypoint.visibility == visibility_left)
		{
			target_direction = lerp(target_direction, collision_avoidance, perp(target_direction));
		}
		else if(closest_waypoint.visibility == visibility_right)
		{
			target_direction = lerp(target_direction, collision_avoidance, -perp(target_direction));
		}
		target_direction = normalize(target_direction);

		if(agent->id == storage->selected_agent)
		{
			V2 line = current_waypoint - agent->p;
			V2 line_n = DEFAULT_AGENT_RADIUS * normalize(v2(-line.y, line.x));
			V4 color_left, color_right;
			if(closest_waypoint.visibility == visibility_direct)
			{
				color_left = color_right = color_white;
			}
			else if(closest_waypoint.visibility == visibility_left)
			{
				color_left = color_white;
				color_right = color_red;
			}
			else
			{
				color_left = color_red;
				color_right = color_white;
			}
			push_line(render_info, agent->p + line_n, current_waypoint + line_n, DEBUG_LINE_THICKNESS, color_left, rendering_priority_sim_overlay);
			push_line(render_info, agent->p - line_n, current_waypoint - line_n, DEBUG_LINE_THICKNESS, color_right, rendering_priority_sim_overlay);
		}
		
		V2 target_velocity = agent->desired_velocity * target_direction;
		V2 force_self = (1.0f / agent->relaxation_time) * (target_velocity - agent->dp);

		V2 ddp = force_self + force_interaction + random_force;
		agent->dp += ddp * dt;
	}
	
	// NOTE: an actual position update is delayed to keep the sim order-independent
	for(u16 i = 0; i < storage->agent_count; ++i)
	{
		Agent *agent = storage->agents + i;
		//
		// NOTE: we should cap the agent's velocity to increase the model stability.
		// However, it just _decreases_ the change of the model freaking out, but doesn't guarantee
		// it'll be working correctly, since there's no proper collision handling.
		// TODO ? remove the _entire_ physical force concept, handle the collisions by manipulating
		// dp directly (it shouldn't change the behaviour conceptually + makes more sense intuitively)
		f32 current_velocity = length(agent->dp);
		f32 dp_len = dt * current_velocity;
		if(dp_len > agent->radius)
		{
			agent->dp = agent->radius * agent->dp / current_velocity;
		}
		//
		agent->p += dt * agent->dp;
		agent->lifetime += dt;

		/*
		AgentFrameNode *frame_node = push_struct(&storage->stats_memory, AgentFrameNode);
		if(frame_node)
		{
			frame_node->t = agent->lifetime;
			frame_node->p = agent->p;
			//frame_node->next = storage->stats[storage->current_run].agent_records[agent->stats_index].frames_sentinel.next;
			//storage->stats[storage->current_run].agent_records[agent->stats_index].frames_sentinel.next = frame_node;
			frame_node->next = 0;
			storage->stats[storage->current_run].agent_records[agent->stats_index].back_sentinel.next->next = frame_node;
			storage->stats[storage->current_run].agent_records[agent->stats_index].back_sentinel.next = frame_node;
		}
		else
		{
			crash(); // TODO: handle it better
		}
		*/
	}
}
