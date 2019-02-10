
s32 get_agent_index(SimStorage *storage, AgentId agent_id)
{
	s32 result = -1;
	u32 hash_index = agent_id.value & (AGENT_ID_HASH_SIZE - 1);
	AgentIdHashNode *hash_slot = storage->agent_id_hash_table + hash_index;
	if(is_valid(hash_slot->id))
	{
		AgentIdHashNode *node = hash_slot;
	    do
		{
			if(node->id == agent_id)
			{
				assert(node->index < storage->agent_count);
			    result = node->index;
				break;
			}
			node = node->next;
		}
		while(node != hash_slot);
	}
	return result;
}

Agent *get_agent(SimStorage *storage, AgentId agent_id)
{
	Agent *result = 0;
	s32 index = get_agent_index(storage, agent_id);
	if(index > -1 && index < storage->agent_count)
	{
		result = storage->agents + index;
	}
	return result;
}

void delete_from_hash(SimStorage *storage, AgentId agent_id)
{
	u32 hash_index = agent_id.value & (AGENT_ID_HASH_SIZE - 1);
	AgentIdHashNode *hash_slot = storage->agent_id_hash_table + hash_index;
	if(is_valid(hash_slot->id))
	{
		if(hash_slot->id == agent_id)
		{
			hash_slot->id.value = 0;
		}
		else
		{
			AgentIdHashNode *node = hash_slot->next;
			while(node != hash_slot)
			{
				if(node->id == agent_id)
				{
					node->prev->next = node->next;
					node->next->prev = node->prev;
					node->next = storage->first_free_node;
					storage->first_free_node = node;
					break;
				}
				node = node->next;
			}
		}
	}
}

void update_hash(SimStorage *storage, AgentId agent_id, u16 agent_index)
{
	u32 hash_index = agent_id.value & (AGENT_ID_HASH_SIZE - 1);
	AgentIdHashNode *hash_slot = storage->agent_id_hash_table + hash_index;
	if(is_valid(hash_slot->id))
	{
		bool found = false;
		AgentIdHashNode *node = hash_slot;

		do
		{
			if(node->id == agent_id)
			{
				node->index = agent_index;
				found = true;
			}
			node = node->next;
		}
		while(node != hash_slot && !found);
		
		if(!found)
		{
			AgentIdHashNode *new_node;
			if(storage->first_free_node)
			{
				new_node = storage->first_free_node;
				storage->first_free_node = new_node->next;
			}
			else
			{
				new_node = push_struct(&storage->persistent_memory, AgentIdHashNode);
			}
			new_node->id = agent_id;
			new_node->index = agent_index;
			new_node->prev = hash_slot;
			new_node->next = hash_slot->next;
			new_node->prev->next = new_node->next->prev = new_node;
		}
	}
	else
	{
	    hash_slot->id = agent_id;
		hash_slot->index = agent_index;
	}
}

inline Agent *add_agent(SimStorage *storage)
{
    Agent *result = 0;
    if(storage->agent_count < MAX_AGENT_COUNT)
    {
		u16 index = storage->agent_count++;
		AgentId id = {++storage->last_agent_id.value};
		
        result = storage->agents + index;
		result->id = id;
		
		assert(id.value); // NOTE: _highly_ unprobable situation

		update_hash(storage, id, index);
    }
    return result;
}

inline Agent *zero_agent(SimStorage *storage)
{
	Agent *result = add_agent(storage);
	AgentId id = result->id;
	zero_struct(result);
	result->id = id;
	
    return result;
}

inline Agent *default_agent(SimStorage *storage)
{
	Agent *result = zero_agent(storage);
	result->radius = storage->state.params.radius;
	result->private_zone_radius = storage->state.params.private_zone_radius;
    result->desired_velocity = storage->state.params.desired_velocity;
    result->relaxation_time = storage->state.params.relaxation_time;
    result->repulsion_coefficient = storage->state.params.repulsion_coefficient;
    result->falloff_length = storage->state.params.falloff_length;
    result->body_force_constant = storage->state.params.body_force_constant;
    result->friction_coefficient = storage->state.params.friction_coefficient;

	result->stats_index = storage->stats[storage->current_run].agent_count++;
	
	result->lifetime = 0;

	return result;
}

inline Agent *copy_agent(SimStorage *storage, Agent *agent)
{
	Agent *result = add_agent(storage);
	AgentId id = result->id;
	*result = *agent;
	result->id = id;
	return result;
}

inline void move_agent(SimStorage *storage, u16 index_from, u16 index_to)
{
	if(index_from != index_to)
	{
		update_hash(storage, storage->agents[index_from].id, index_to);
		storage->agents[index_to] = storage->agents[index_from];
	}
}

inline void move_agent(SimStorage *storage, Agent *agent, u16 index_to)
{
	u16 agent_index = (u16)(agent - storage->agents);
	move_agent(storage, agent_index, index_to);
}

inline void place_agent(SimStorage *storage, Agent *agent, V2 p)
{
	Agent copy = *agent;
	copy.dp = v2();
	copy.p = p;
    copy.last_waypoint_update = 0;
	copy.current_waypoint.p = p;
	copy.current_waypoint.visibility = visibility_none;

	/*
	storage->stats[storage->current_run].agent_records[copy.stats_index].lifetime = 0;
	AgentFrameNode *frame_node = push_struct(&storage->stats_memory, AgentFrameNode);
	if(frame_node)
	{
		frame_node->t = 0;
		frame_node->p = p;
		frame_node->next = 0;
		//storage->stats[storage->current_run].agent_records[agent->stats_index].frames_sentinel.next = frame_node;
		storage->stats[storage->current_run].agent_records[agent->stats_index].front_sentinel.next =
			storage->stats[storage->current_run].agent_records[agent->stats_index].back_sentinel.next =
			frame_node;
	}
	*/
	
	s32 partition_index = get_partition_index(storage, p);
	if(partition_index >= 0)
	{
		// all the partitions _after_ the one we need get shifted up one index
		// so we can insert the agent in the empty slot
		for(s32 i = array_count(storage->sim_grid) - 1; i > partition_index; --i)
		{
			WorldPartition *partition = storage->sim_grid + i;
			if(!is_empty(partition))
			{
				move_agent(storage, partition->first_agent_index, partition->one_over_last_agent_index);
			}
			++partition->first_agent_index;
			++partition->one_over_last_agent_index;
		}
	}
	else
	{
		partition_index = array_count(storage->sim_grid) - 1;
	}
	WorldPartition *partition = storage->sim_grid + partition_index;
	u16 new_index = partition->one_over_last_agent_index++;
	storage->agents[new_index] = copy;
	update_hash(storage, copy.id, new_index);
}

s32 spawn_random_agents(SimStorage *storage, u32 count)
{
	begin_tmp_memory(&storage->scratch_memory);
	u32 placed_agent_count = 0;
	u32 position_count = storage->available_position_count;
	V2 *positions = push_array(&storage->scratch_memory, V2, position_count);
    array_copy(storage->available_positions, positions, V2, position_count);
	count = min(count, position_count);
	while(placed_agent_count < count && position_count > 0)
	{
		V2 p = {};
		bool occupied;

		do
		{
			occupied = false;
			s32 index = random_between_i(&storage->rng, 0, position_count - 1);
			p = positions[index];
			positions[index] = positions[--position_count];

			for(u32 i = 0; i < storage->agent_count && !occupied; ++i)
			{
				Agent *agent = storage->agents + i;
				occupied = intersect(p, DEFAULT_AGENT_RADIUS, agent->p, agent->radius);
			}
		}
		while(occupied && position_count > 0);

		if(!occupied)
		{
			Agent *agent = default_agent(storage);
			place_agent(storage, agent, p);
			++placed_agent_count;
		}
	}
	storage->stats[storage->current_run].agent_count += placed_agent_count;
	return placed_agent_count;
}
